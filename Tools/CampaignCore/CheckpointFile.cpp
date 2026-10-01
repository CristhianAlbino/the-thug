#include "CheckpointFile.h"
#include "CampaignIO.h"
#include <atomic>
#include <cerrno>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace thug::campaign::checkpoint {
namespace {
namespace fs = std::filesystem;
#ifdef _WIN32
using Handle = HANDLE;
const Handle Invalid = INVALID_HANDLE_VALUE;
int LastError() { return static_cast<int>(GetLastError()); }
void Close(Handle handle) { if (handle != Invalid) CloseHandle(handle); }
std::uint64_t Process() { return GetCurrentProcessId(); }
#else
using Handle = int;
constexpr Handle Invalid = -1;
int LastError() { return errno; }
void Close(Handle handle) { if (handle != Invalid) close(handle); }
std::uint64_t Process() { return static_cast<std::uint64_t>(getpid()); }
#endif
struct OwnedHandle {
  Handle value = Invalid;
  ~OwnedHandle() { Close(value); }
  OwnedHandle() = default;
  OwnedHandle(const OwnedHandle&) = delete;
  OwnedHandle& operator=(const OwnedHandle&) = delete;
};
struct SlotLock {
  OwnedHandle file;
  Result result{Status::Ok};
  explicit SlotLock(const fs::path& path) {
#ifdef _WIN32
    file.value = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.value == Invalid) { result = {Status::IoError, save::Result::Ok, LastError()}; return; }
    OVERLAPPED offset{};
    if (!LockFileEx(file.value, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &offset)) {
      const int error = LastError();
      result = {error == ERROR_LOCK_VIOLATION ? Status::Busy : Status::IoError, save::Result::Ok, error};
    }
#else
    file.value = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    if (file.value == Invalid) { result = {Status::IoError, save::Result::Ok, LastError()}; return; }
    if (flock(file.value, LOCK_EX | LOCK_NB)) {
      const int error = LastError();
      result = {error == EWOULDBLOCK || error == EAGAIN ? Status::Busy : Status::IoError, save::Result::Ok, error};
    }
#endif
  }
  ~SlotLock() {
    if (result.status != Status::Ok || file.value == Invalid) return;
#ifdef _WIN32
    OVERLAPPED offset{}; UnlockFileEx(file.value, 0, 1, 0, &offset);
#else
    flock(file.value, LOCK_UN);
#endif
  }
};
struct Temp {
  fs::path path;
  OwnedHandle file;
  ~Temp() { Close(file.value); file.value = Invalid; if (!path.empty()) { std::error_code error; fs::remove(path, error); } }
};
fs::path Suffix(fs::path path, const std::string& suffix) { path += fs::path(suffix); return path; }
Result PathValid(const fs::path& path) {
  if (path.empty() || path.filename().empty() || path.filename() == "." || path.filename() == "..") return {Status::InvalidPath};
  std::error_code error;
  const auto parent = path.has_parent_path() ? path.parent_path() : fs::path(".");
  if (!fs::is_directory(parent, error)) return {Status::InvalidPath, save::Result::Ok, error.value()};
  return {Status::Ok};
}
bool CreateTemp(const fs::path& slot, Temp& temp, int& error) {
  static std::atomic<std::uint64_t> serial{0};
  for (int attempt = 0; attempt < 100; ++attempt) {
    temp.path = Suffix(slot, ".thug-tmp." + std::to_string(Process()) + "." + std::to_string(++serial));
#ifdef _WIN32
    temp.file.value = CreateFileW(temp.path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
                                  nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (temp.file.value != Invalid) return true;
    error = LastError(); if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) { temp.path.clear(); return false; }
#else
    temp.file.value = open(temp.path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (temp.file.value != Invalid) return true;
    error = LastError(); if (error != EEXIST) { temp.path.clear(); return false; }
#endif
    temp.path.clear(); // Never delete an exclusive-create collision owned by someone else.
  }
  return false;
}
bool Write(Handle file, const std::string& bytes, int& error) {
  std::size_t written = 0;
  while (written < bytes.size()) {
#ifdef _WIN32
    DWORD count = 0;
    if (!WriteFile(file, bytes.data() + written, static_cast<DWORD>(bytes.size() - written), &count, nullptr) || count == 0) { error = LastError(); return false; }
#else
    const auto count = write(file, bytes.data() + written, bytes.size() - written);
    if (count < 0 && errno == EINTR) continue;
    if (count <= 0) { error = LastError(); return false; }
#endif
    written += static_cast<std::size_t>(count);
  }
  return true;
}
bool Sync(Handle file, int& error) {
#ifdef _WIN32
  if (FlushFileBuffers(file)) return true;
#else
  while (fsync(file)) { if (errno == EINTR) continue; error = LastError(); return false; }
  return true;
#endif
#ifdef _WIN32
  error = LastError(); return false;
#endif
}
bool CloseChecked(OwnedHandle& file, int& error) {
  const auto handle = file.value; file.value = Invalid;
#ifdef _WIN32
  if (CloseHandle(handle)) return true;
#else
  if (close(handle) == 0) return true;
#endif
  error = LastError(); return false;
}
bool Replace(Temp& from, const fs::path& to, int& error) {
#ifdef _WIN32
  if (!MoveFileExW(from.path.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) { error = LastError(); return false; }
#else
  if (rename(from.path.c_str(), to.c_str())) { error = LastError(); return false; }
#endif
  from.path.clear(); return true;
}
bool SyncDirectory(const fs::path& slot, int& error) {
#ifdef _WIN32
  // Replacement requests write-through; temporary and backup data were flushed
  // individually. This is the Windows backend, not a POSIX directory fsync.
  (void)slot; (void)error; return true;
#else
  OwnedHandle directory;
  const auto parent = slot.has_parent_path() ? slot.parent_path() : fs::path(".");
  directory.value = open(parent.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
  if (directory.value == Invalid) { error = LastError(); return false; }
  return Sync(directory.value, error);
#endif
}
Result Read(const fs::path& path, std::string& bytes) {
  std::error_code error;
  if (!fs::exists(path, error)) return {error ? Status::IoError : Status::NotFound, save::Result::Ok, error.value()};
  if (!fs::is_regular_file(path, error)) return {Status::InvalidPath, save::Result::Ok, error.value()};
  std::ifstream in(path, std::ios::binary);
  if (!in) return {Status::IoError, save::Result::IoError};
  char chunk[4096];
  while (in.read(chunk, sizeof(chunk)) || in.gcount()) {
    if (bytes.size() + static_cast<std::size_t>(in.gcount()) > detail::MaxRuntimeBytes) return {Status::InvalidCheckpoint, save::Result::TooLarge};
    bytes.append(chunk, static_cast<std::size_t>(in.gcount()));
  }
  return in.eof() && !in.bad() ? Result{Status::Ok} : Result{Status::IoError, save::Result::IoError};
}
save::Result Validate(const CampaignCore& core, const std::string& bytes) { std::istringstream in(bytes); return core.ValidateCheckpoint(in); }
bool Stop(const Observer& observer, Stage stage) { return observer && !observer(stage); }
void CleanupOrphans(const fs::path& slot) {
  std::error_code error;
  const auto parent = slot.has_parent_path() ? slot.parent_path() : fs::path(".");
  const auto primary = Suffix(slot.filename(), ".thug-tmp.").native();
  const auto backup = Suffix(slot.filename(), ".bak.thug-tmp.").native();
  for (fs::directory_iterator it(parent, error), end; !error && it != end; it.increment(error)) {
    const auto name = it->path().filename().native();
    const auto prefix = name.compare(0, primary.size(), primary) == 0 ? primary.size() :
                        name.compare(0, backup.size(), backup) == 0 ? backup.size() : 0;
    if (!prefix) continue;
    bool valid = false;
    for (std::size_t i = prefix; i < name.size(); ++i) {
      if (name[i] == '.' && !valid && i > prefix && i + 1 < name.size()) valid = true;
      else if (name[i] < '0' || name[i] > '9') { valid = false; break; }
    }
    if (valid) { std::error_code ignored; fs::remove(it->path(), ignored); }
  }
}
}

Result SaveFile(const CampaignCore& core, const fs::path& slot, const Observer& observer) {
  const auto validPath = PathValid(slot); if (validPath.status != Status::Ok) return validPath;
  SlotLock lock(Suffix(slot, ".lock")); if (lock.result.status != Status::Ok) return lock.result;
  CleanupOrphans(slot);
  std::ostringstream out;
  const auto serialized = core.Save(out); if (serialized != save::Result::Ok) return {Status::InvalidCheckpoint, serialized};
  const auto bytes = out.str();
  std::string previous;
  const auto read = Read(slot, previous);
  if (read.status == Status::IoError || read.status == Status::InvalidPath) return read;
  const auto previousValid = read.status == Status::Ok ? Validate(core, previous) : read.checkpoint;
  if (previousValid == save::Result::UnsupportedVersion || previousValid == save::Result::IncompatibleRoster) return {Status::InvalidCheckpoint, previousValid};
  int error = 0;
  Temp temp;
  if (!CreateTemp(slot, temp, error)) return {Status::IoError, save::Result::Ok, error};
  if (Stop(observer, Stage::TempCreated)) return {Status::Interrupted};
  if (!Write(temp.file.value, bytes, error)) return {Status::IoError, save::Result::Ok, error};
  if (Stop(observer, Stage::PayloadWritten)) return {Status::Interrupted};
  if (!Sync(temp.file.value, error) || !CloseChecked(temp.file, error)) return {Status::IoError, save::Result::Ok, error};
  std::string verified;
  const auto onDisk = Read(temp.path, verified);
  if (onDisk.status != Status::Ok || verified != bytes || Validate(core, verified) != save::Result::Ok) return {Status::InvalidCheckpoint, save::Result::Corrupt};
  if (Stop(observer, Stage::PayloadSynced)) return {Status::Interrupted};
  if (read.status == Status::Ok && previousValid == save::Result::Ok) {
    const auto backup = Suffix(slot, ".bak");
    Temp prior;
    if (!CreateTemp(backup, prior, error) || !Write(prior.file.value, previous, error) || !Sync(prior.file.value, error) ||
        !CloseChecked(prior.file, error)) return {Status::IoError, save::Result::Ok, error};
    std::string backupBytes;
    if (Read(prior.path, backupBytes).status != Status::Ok || backupBytes != previous || Validate(core, backupBytes) != save::Result::Ok)
      return {Status::InvalidCheckpoint, save::Result::Corrupt};
    if (!Replace(prior, backup, error) || !SyncDirectory(backup, error)) return {Status::IoError, save::Result::Ok, error};
  }
  if (Stop(observer, Stage::BackupSynced)) return {Status::Interrupted};
  if (!Replace(temp, slot, error)) return {Status::IoError, save::Result::Ok, error};
  if (Stop(observer, Stage::PrimaryReplaced)) return {Status::CommittedUnconfirmed};
  if (!SyncDirectory(slot, error)) return {Status::CommittedUnconfirmed, save::Result::Ok, error};
  if (observer) observer(Stage::DirectorySynced); // Already committed: cancellation cannot undo durability.
  return {Status::Ok};
}
Result LoadFile(CampaignCore& core, const fs::path& slot) {
  const auto validPath = PathValid(slot); if (validPath.status != Status::Ok) return validPath;
  SlotLock lock(Suffix(slot, ".lock")); if (lock.result.status != Status::Ok) return lock.result;
  std::string bytes;
  const auto primary = Read(slot, bytes);
  save::Result failed = primary.checkpoint;
  if (primary.status == Status::Ok) {
    std::istringstream in(bytes); failed = core.Load(in);
    if (failed == save::Result::Ok) return {Status::Ok};
    if (failed == save::Result::UnsupportedVersion || failed == save::Result::IncompatibleRoster) return {Status::InvalidCheckpoint, failed};
  } else if (primary.status == Status::InvalidPath) return primary;
  bytes.clear();
  const auto backup = Read(Suffix(slot, ".bak"), bytes);
  if (backup.status == Status::Ok) {
    std::istringstream in(bytes); const auto loaded = core.Load(in);
    if (loaded == save::Result::Ok) return {Status::RecoveredBackup};
    return {Status::InvalidCheckpoint, loaded};
  }
  if (primary.status == Status::NotFound && backup.status == Status::NotFound) return {Status::NotFound};
  return primary.status == Status::Ok || primary.status == Status::InvalidCheckpoint ? Result{Status::InvalidCheckpoint, failed} : primary;
}
} // namespace thug::campaign::checkpoint
