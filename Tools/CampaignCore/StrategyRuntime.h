#pragma once
#include "CompanionRuntime.h"

namespace thug::campaign {
inline constexpr const char* StrategyRevision = "resources-information-cooperation-1";
inline constexpr double QuestionSeconds = 4, JointSeconds = 3, ReportFreshSeconds = 10;
enum class StrategyOp {
  DropItem, NoticeDrop, TakeDrop, NoticeMissingDrop, ObserveArea, ReportThreat, CheckReport,
  BeginQuestion, AnswerQuestion, CancelQuestion,
  BeginJointUse, CompleteJointUse, CancelJointUse,
  NoticeSmoke, BeginTrap, BeginSmokeWatch, BeginRetreat, ArrivePlan, CompletePlanStep, CancelPlan
};
enum class AnswerKind { Refuse, RevealRoute, Mislead };
struct StrategyCommand {
  std::string eventId;
  StrategyOp kind = StrategyOp::ObserveArea;
  std::string actor = "joe", target, node, item, destination, reference;
  double atSeconds = 0;
  int argument = 0; // Quantity; perceived threat 0/1; deliberate lie 0/1; AnswerKind.
  std::uint64_t serial = 0;
  hunter::Vec3 position{}; // Confirmed resting position of a dropped stack, meters.
  bool physicallyVerified = false;
};
struct StrategyContext {
  std::map<std::string, director::Actor> actors; // Only perceived participants.
  std::string actualHunterNode;
  bool actionCompleted = false, areaObserved = false, speechPerceived = false;
  bool contactVerified = false, continuityVerified = false;
  bool movementCompleted = false, lineOfSightVerified = false, reactionCueVerified = false;
  bool alternativePathVerified = false;
  std::set<std::string> reachableNodes{}, safeToAlter{}, observedCoverNodes{};
  std::map<std::string,std::set<std::string>> observedSmokeEdges{};
};
struct DroppedStack {
  std::string owner, item, node;
  int quantity = 0;
  hunter::Vec3 position{};
  std::set<std::string> knownTo;
};
struct KnownDrop {
  std::string item, node;
  int quantity = 0; // Last observed amount, not the private current world amount.
  hunter::Vec3 position{};
  double atSeconds = 0;
};
struct AreaObservation {
  std::string eventId, node;
  double atSeconds = 0;
  bool hunterPresent = false;
};
struct InformationReport {
  std::string source, recipient, node, route;
  double atSeconds = 0;
  bool hunterPresent = false, deliberateLie = false, checked = false, contradicted = false;
};
// Player-facing view deliberately excludes the speaker's private intention.
struct KnownReport {
  std::string id, source, node, route;
  double atSeconds;
  bool hunterPresent, checked, contradicted, stale;
};
struct StrategyTask {
  std::uint64_t serial = 0, actorHarm = 0, peerHarm = 0;
  StrategyOp kind = StrategyOp::BeginQuestion;
  std::string actor, peer, target, node, startEventId;
  double startedAt = 0, readyAt = 0;
  bool interrupted = false;
};
struct SmokePattern { unsigned uses = 0; double lastAt = 0; };
struct HunterPlan {
  std::uint64_t serial = 0, harm = 0;
  StrategyOp kind = StrategyOp::BeginTrap;
  std::string target, destination, auxiliary, startEventId;
  unsigned step = 0;
  double startedAt = 0, arrivedAt = -1;
  bool interrupted = false;
};
struct HunterPlanOffer {
  StrategyOp kind;
  std::string target, destination, auxiliary;
  double weight;
};
struct StrategyRuntimeSnapshot {
  std::uint64_t serial = 0;
  std::map<std::string, DroppedStack> drops; // Includes empty tombstones for stale interaction IDs.
  std::map<std::string,std::map<std::string,KnownDrop>> knownDrops;
  std::map<std::string, std::map<std::string, AreaObservation>> observations;
  std::map<std::string, InformationReport> reports;
  std::optional<StrategyTask> question, joint;
  std::map<std::uint64_t,std::string> seenSmoke;
  std::map<std::string,SmokePattern> smokePatterns;
  std::optional<HunterPlan> hunterPlan;
  double lastPlanAt = -1;
};
} // namespace thug::campaign
