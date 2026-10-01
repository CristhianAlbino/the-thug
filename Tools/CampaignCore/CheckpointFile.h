#pragma once
#include "CampaignCore.h"
#include <filesystem>
#include <functional>

namespace thug::campaign::checkpoint {
enum class Status { Ok, RecoveredBackup, NotFound, InvalidPath, InvalidCheckpoint, IoError, Busy, Interrupted, CommittedUnconfirmed };
struct Result {
  Status status;
  save::Result checkpoint = save::Result::Ok;
  int systemError = 0;
};
enum class Stage { TempCreated, PayloadWritten, PayloadSynced, BackupSynced, PrimaryReplaced, DirectorySynced };
// Called with the slot lock held. False cancels before replacement; after
// replacement it reports an unconfirmed commit. Callback must not throw/reenter.
using Observer = std::function<bool(Stage)>;
Result SaveFile(const CampaignCore&, const std::filesystem::path& slot, const Observer& observer = {});
// Recovers corruption/missing primary from .bak, with an explicit status. Future
// versions or incompatible content never fall back silently to older progress.
Result LoadFile(CampaignCore&, const std::filesystem::path& slot);
} // namespace thug::campaign::checkpoint
