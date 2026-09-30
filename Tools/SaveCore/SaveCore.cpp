#include "SaveCore.h"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace thug::save {
namespace {

constexpr std::size_t MaxFileBytes = 2 * 1024 * 1024;
constexpr unsigned MaxEntries = 10000;
constexpr std::size_t MaxIdBytes = 1024;
constexpr std::size_t MaxRandomBytes = 16000;
constexpr int Version = 1;

static_assert(std::is_nothrow_swappable_v<survival::Inventory>);
static_assert(std::is_nothrow_swappable_v<companion::CompanionCore>);

bool CorrectRoster(const Party& party) {
  if (&party.harley == &party.bartholomeu ||
      &party.harley == &party.jack ||
      &party.bartholomeu == &party.jack ||
      party.harley.Identity() != companion::Character::Harley ||
      party.bartholomeu.Identity() != companion::Character::Bartholomeu ||
      party.jack.Identity() != companion::Character::Jack) return false;
  const auto a = party.harley.DifficultyLevel();
  return a == party.bartholomeu.DifficultyLevel() &&
         a == party.jack.DifficultyLevel();
}

bool FitsId(const std::string& id) {
  return !id.empty() && id.size() <= MaxIdBytes;
}

bool FitsFormat(const survival::InventorySnapshot& inventory,
                const companion::CompanionSnapshot& harley,
                const companion::CompanionSnapshot& bartholomeu,
                const companion::CompanionSnapshot& jack) {
  if (inventory.definitions.size() > MaxEntries ||
      inventory.contents.size() > MaxEntries) return false;
  for (const auto& item : inventory.definitions)
    if (!FitsId(item.id)) return false;
  for (const auto& [id, count] : inventory.contents)
    if (!FitsId(id)) return false;
  for (const auto* companion : {&harley, &bartholomeu, &jack}) {
    if (companion->knownRoutes.size() > MaxEntries ||
        companion->appliedEvents.size() > MaxEntries ||
        companion->randomState.empty() ||
        companion->randomState.size() > MaxRandomBytes) return false;
    for (const auto& id : companion->knownRoutes)
      if (!FitsId(id)) return false;
    for (const auto& id : companion->appliedEvents)
      if (!FitsId(id)) return false;
  }
  return true;
}

std::string Checksum(const std::string& bytes) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (unsigned char byte : bytes) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

bool Tag(std::istream& in, const char* expected) {
  std::string value;
  return static_cast<bool>(in >> value) && value == expected;
}

bool ReadString(std::istream& in, std::string& value,
                std::size_t maxLength = MaxIdBytes) {
  in >> std::ws;
  if (in.peek() != '"' || !(in >> std::quoted(value))) return false;
  return !value.empty() && value.size() <= maxLength;
}

bool ReadCount(std::istream& in, unsigned& count) {
  return static_cast<bool>(in >> count) && count <= MaxEntries;
}

bool ReadBool(std::istream& in, bool& value) {
  int number;
  if (!(in >> number) || (number != 0 && number != 1)) return false;
  value = number == 1;
  return true;
}

bool ValidCategory(int value) { return value >= 0 && value <= 5; }
bool ValidCharacter(int value) { return value >= 0 && value <= 2; }
bool ValidDifficulty(int value) { return value >= 0 && value <= 3; }

void WriteInventory(std::ostream& out,
                    const survival::InventorySnapshot& snapshot) {
  out << "INVENTORY " << snapshot.capacitySlots << ' '
      << snapshot.definitions.size() << ' ' << snapshot.contents.size() << '\n';
  for (const auto& item : snapshot.definitions) {
    out << "ITEM " << std::quoted(item.id) << ' '
        << static_cast<int>(item.category) << ' ' << item.slotCost << ' '
        << item.maxQuantity << ' ' << static_cast<int>(item.droppable) << '\n';
  }
  for (const auto& [id, count] : snapshot.contents) {
    out << "STOCK " << std::quoted(id) << ' ' << count << '\n';
  }
}

bool ReadInventory(std::istream& in, survival::InventorySnapshot& snapshot) {
  unsigned definitions, contents;
  if (!Tag(in, "INVENTORY") || !(in >> snapshot.capacitySlots) ||
      !ReadCount(in, definitions) || !ReadCount(in, contents) ||
      snapshot.capacitySlots < 0) return false;
  for (unsigned i = 0; i < definitions; ++i) {
    survival::ItemDefinition item;
    int category;
    if (!Tag(in, "ITEM") || !ReadString(in, item.id) ||
        !(in >> category >> item.slotCost >> item.maxQuantity) ||
        !ReadBool(in, item.droppable) || !ValidCategory(category) ||
        item.slotCost <= 0 || item.maxQuantity <= 0) return false;
    item.category = static_cast<survival::ItemCategory>(category);
    snapshot.definitions.push_back(std::move(item));
  }
  for (unsigned i = 0; i < contents; ++i) {
    std::string id;
    int count;
    if (!Tag(in, "STOCK") || !ReadString(in, id) || !(in >> count) ||
        count <= 0 || !snapshot.contents.emplace(std::move(id), count).second)
      return false;
  }
  return true;
}

void WriteCompanion(std::ostream& out,
                    const companion::CompanionSnapshot& snapshot) {
  const auto& t = snapshot.traits;
  out << "COMPANION " << static_cast<int>(snapshot.character) << ' '
      << static_cast<int>(snapshot.difficulty) << '\n'
      << "TRAITS " << t.trust << ' ' << t.fear << ' ' << t.stress << ' '
      << t.loyalty << ' ' << t.selfPreservation << ' ' << t.relationship << ' '
      << t.ammunition << ' ' << static_cast<int>(t.injured) << ' '
      << snapshot.nowSeconds << '\n'
      << "RNG " << std::quoted(snapshot.randomState) << '\n'
      << "ROUTES " << snapshot.knownRoutes.size() << '\n';
  for (const auto& id : snapshot.knownRoutes)
    out << "ROUTE " << std::quoted(id) << '\n';
  out << "EVENTS " << snapshot.appliedEvents.size() << '\n';
  for (const auto& id : snapshot.appliedEvents)
    out << "EVENT " << std::quoted(id) << '\n';
}

bool ReadCompanion(std::istream& in,
                   companion::CompanionSnapshot& snapshot) {
  int character, difficulty;
  if (!Tag(in, "COMPANION") || !(in >> character >> difficulty) ||
      !ValidCharacter(character) || !ValidDifficulty(difficulty)) return false;
  snapshot.character = static_cast<companion::Character>(character);
  snapshot.difficulty = static_cast<companion::Difficulty>(difficulty);
  auto& t = snapshot.traits;
  if (!Tag(in, "TRAITS") ||
      !(in >> t.trust >> t.fear >> t.stress >> t.loyalty >>
        t.selfPreservation >> t.relationship >> t.ammunition) ||
      !ReadBool(in, t.injured) || !(in >> snapshot.nowSeconds) ||
      !Tag(in, "RNG") || !ReadString(in, snapshot.randomState, MaxRandomBytes))
    return false;
  unsigned routes;
  if (!Tag(in, "ROUTES") || !ReadCount(in, routes)) return false;
  for (unsigned i = 0; i < routes; ++i) {
    std::string id;
    if (!Tag(in, "ROUTE") || !ReadString(in, id) ||
        !snapshot.knownRoutes.insert(std::move(id)).second) return false;
  }
  unsigned events;
  if (!Tag(in, "EVENTS") || !ReadCount(in, events)) return false;
  for (unsigned i = 0; i < events; ++i) {
    std::string id;
    if (!Tag(in, "EVENT") || !ReadString(in, id) ||
        !snapshot.appliedEvents.insert(std::move(id)).second) return false;
  }
  return true;
}

Result ReadBounded(std::istream& input, std::string& bytes) {
  char c;
  while (input.get(c)) {
    if (bytes.size() == MaxFileBytes) return Result::TooLarge;
    bytes.push_back(c);
  }
  return input.eof() && !input.bad() ? Result::Ok : Result::IoError;
}

} // namespace

Result SaveParty(std::ostream& output, const Party& party) {
  if (!CorrectRoster(party)) return Result::IncompatibleRoster;
  const auto inventory = party.joe.CaptureSnapshot();
  const auto harley = party.harley.CaptureSnapshot();
  const auto bartholomeu = party.bartholomeu.CaptureSnapshot();
  const auto jack = party.jack.CaptureSnapshot();
  if (!FitsFormat(inventory, harley, bartholomeu, jack))
    return Result::TooLarge;
  std::ostringstream buffer;
  buffer.imbue(std::locale::classic());
  buffer << std::setprecision(std::numeric_limits<double>::max_digits10)
         << "THE_THUG_PARTY " << Version << '\n';
  WriteInventory(buffer, inventory);
  WriteCompanion(buffer, harley);
  WriteCompanion(buffer, bartholomeu);
  WriteCompanion(buffer, jack);
  buffer << "END\n";
  const auto payload = buffer.str();
  const auto data = payload + "CHECKSUM " + Checksum(payload) + '\n';
  if (data.size() > MaxFileBytes) return Result::TooLarge;
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  return output ? Result::Ok : Result::IoError;
}

Result LoadParty(std::istream& input, Party party) {
  if (!CorrectRoster(party)) return Result::IncompatibleRoster;
  std::string data;
  const auto readResult = ReadBounded(input, data);
  if (readResult != Result::Ok) return readResult;
  std::istringstream in(data);
  in.imbue(std::locale::classic());
  int version;
  if (!Tag(in, "THE_THUG_PARTY") || !(in >> version)) return Result::Malformed;
  if (version != Version) return Result::UnsupportedVersion;

  survival::InventorySnapshot inventory;
  companion::CompanionSnapshot harley, bartholomeu, jack;
  if (!ReadInventory(in, inventory) || !ReadCompanion(in, harley) ||
      !ReadCompanion(in, bartholomeu) || !ReadCompanion(in, jack) ||
      !Tag(in, "END")) return Result::Malformed;
  std::string endLine;
  if (!std::getline(in, endLine) || !endLine.empty()) return Result::Malformed;
  const auto checksumOffset = in.tellg();
  if (checksumOffset < 0) return Result::Malformed;
  std::string checksum;
  if (!Tag(in, "CHECKSUM") || !(in >> checksum) || checksum.size() != 16)
    return Result::Malformed;
  in >> std::ws;
  if (!in.eof()) return Result::Malformed;
  if (checksum != Checksum(data.substr(0,
          static_cast<std::size_t>(checksumOffset)))) return Result::Corrupt;

  // Stage all changes; a failure in any state leaves every live object intact.
  auto stagedJoe = party.joe;
  auto stagedHarley = party.harley;
  auto stagedBartholomeu = party.bartholomeu;
  auto stagedJack = party.jack;
  if (!stagedJoe.RestoreSnapshot(inventory) ||
      !stagedHarley.RestoreSnapshot(harley) ||
      !stagedBartholomeu.RestoreSnapshot(bartholomeu) ||
      !stagedJack.RestoreSnapshot(jack)) return Result::IncompatibleRoster;

  using std::swap;
  swap(party.joe, stagedJoe);
  swap(party.harley, stagedHarley);
  swap(party.bartholomeu, stagedBartholomeu);
  swap(party.jack, stagedJack);
  return Result::Ok;
}

} // namespace thug::save
