#pragma once
#include "CompanionRuntime.h"

namespace thug::campaign {
struct CompanionBody {
  CompanionBody(int capacity, const std::vector<survival::ItemDefinition>& items, survival::Tuning tuning)
      : inventory(capacity, items), survival(tuning) {}
  survival::Inventory inventory;
  survival::SurvivalCore survival;
  std::set<std::string> knownPickups, knownHides;
  std::string lastNode, hidingPlace;
  std::uint64_t serial = 0;
  std::optional<CompanionTask> task;
  TaskStop lastStop = TaskStop::None;
  void Stop(TaskStop reason) { if (task) { task.reset(); lastStop = reason; } }
  CompanionRuntimeSnapshot Snapshot() const {
    return {inventory.CaptureSnapshot(), survival.CaptureSnapshot(), knownPickups,
      knownHides, lastNode, hidingPlace, serial, task, lastStop};
  }
};
} // namespace thug::campaign
