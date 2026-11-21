#pragma once
#include "../HunterCore/HunterCore.h"
#include "../HunterCore/RunRules.h"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace thug::campaign {
// Revision participates in checkpoint content identity. Values are playtest hypotheses.
inline constexpr const char* HunterRuntimeRevision = "hunter-tactics-1";
enum class HunterOp { SeeActor, LoseActor, Pressure, NoticePassage, Choose, Arrive, Complete, Cancel };
enum class HunterPressure { Gunshot, Explosion, Destruction, ConfirmedEscape };
enum class HunterGoal { Pursue, Investigate, InspectHide, WatchPassage, CloseDoor, OpenDoor, PowerOff, DisableCamera, Patrol, BreakWedge };
struct HunterTuning {
  double visualLease, aggressionDecay, reactionSeconds, ambushCooldown, watchSeconds;
  double environmentThreshold, ambushWeight;
};
HunterTuning HunterProfile(rules::Difficulty);
struct HunterTrack {
  std::string node;
  hunter::LocationEstimate estimate;
  bool visible = false;
  bool threatening = false;
};
struct HunterTask {
  std::uint64_t serial = 0;
  HunterGoal goal = HunterGoal::Patrol;
  std::string target, destination, startEventId;
  double startedAt = 0, arrivedAt = -1;
};
struct HunterRuntimeSnapshot {
  double nowSeconds = 0, aggression = 0, lastAmbushAt = -1;
  std::uint64_t serial = 0;
  std::map<std::string, HunterTrack> actors;
  std::map<std::string, std::string> latestSightings;
  std::map<std::string, double> lostAt, passages, searched;
  std::map<int, double> pressureAt;
  std::optional<HunterTask> task;
  std::string watchingDoor, randomState;
};
struct HunterCommand {
  std::string eventId;
  HunterOp kind = HunterOp::Choose;
  std::string actor, target, node;
  double atSeconds = 0;
  std::uint64_t taskSerial = 0;
  int argument = 0; // SeeActor: threatening 0/1; Pressure: HunterPressure.
  hunter::Vec3 observedPosition{};
  double radiusMeters = 0;
};
struct HunterContext {
  std::string actualHunterNode;
  std::set<std::string> reachableNodes;
  // World/navigation adapter maps a perceived uncertainty region into sectors.
  // Centers outside that region are ignored. Never pass Joe's hidden transform.
  std::map<std::string, hunter::Vec3> searchSectors;
  // Alternative routes verified before proposing a blocking environment action.
  std::set<std::string> safeToAlter;
  bool physicallyVerified = false;
  bool lineOfSightVerified = false;
  bool alternativePathVerified = false;
  bool reactionCueVerified = false; // World cue actually presented before starting its window.
};
struct HunterOffer {
  HunterGoal goal;
  std::string target, destination;
  double weight;
};
} // namespace thug::campaign
