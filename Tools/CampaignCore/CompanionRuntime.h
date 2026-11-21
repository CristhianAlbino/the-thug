#pragma once
#include "../CompanionCore/CompanionCore.h"
#include "../DirectorCore/DirectorCore.h"
#include "../SurvivalCore/Inventory.h"
#include "../SurvivalCore/SurvivalCore.h"
#include <array>

namespace thug::campaign {
enum class CompanionOp {
  ObservePickup, ObserveHide, ObserveRoute, ChooseTask, BeginTask,
  Arrive, Complete, Cancel, Transfer, Treat, Stabilize, Injury,
  LeaveHide, Operate, Abandon, Outcome, ObserveMissingPickup
};
enum class TaskPhase { Travelling, Ready };
enum class TaskStop { None, Completed, Cancelled, Injured, Collapsed, Outcome, CampaignEnded };
enum class CompanionOutcome { Captured, Dead, Together };
struct CompanionTask {
  std::uint64_t serial = 0;
  companion::Goal goal = companion::Goal::StayHidden;
  std::string target;
  std::string destination;
  std::string item;
  TaskPhase phase = TaskPhase::Travelling;
  double startedAt = 0;
  double readyAt = -1;
  std::string startEventId{};
};
struct CompanionOffer {
  companion::Goal goal;
  double weight;
  std::string target;
  std::string destination;
  std::string item;
};
struct CompanionCommand {
  std::string eventId;
  CompanionOp kind = CompanionOp::ObservePickup;
  std::string actor;
  std::string target;
  std::string node;
  double atSeconds = 0;
  std::string item{};
  int quantity = 0;
  std::uint64_t taskSerial = 0;
  int argument = 0;
  bool physicallyVerified = false;
};
// Only actors actually perceived by this companion belong here. Reachable
// nodes are verified paths, not a global navigation/knowledge oracle.
struct CompanionContext {
  std::map<std::string, director::Actor> actors;
  std::set<std::string> reachableNodes;
  bool hunterPerceived = false;
  bool joeNeedsHelpObserved = false;
  bool movementCompleted = false;
  bool actionCompleted = false;
  bool joeWitnessedAction = false;
  bool inDanger = false;
  hunter::Vec3 actualSourceMeters{};
};
struct CompanionRuntimeSnapshot {
  survival::InventorySnapshot inventory;
  survival::SurvivalSnapshot survival;
  std::set<std::string> knownPickups;
  std::set<std::string> knownHides;
  std::string lastNode;
  std::string hidingPlace;
  std::uint64_t serial = 0;
  std::optional<CompanionTask> task;
  TaskStop lastStop = TaskStop::None;
};
struct CompanionSignal {
  std::string actor;
  std::string node;
  perception::SoundKind sound;
  hunter::Vec3 actualSourceMeters;
  double atSeconds;
};
struct CompanionProfile {
  std::array<int, 3> capacities;
  std::array<std::map<std::string, int>, 3> loadouts;
};
} // namespace thug::campaign
