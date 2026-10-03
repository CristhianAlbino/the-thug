#pragma once

#include "../PerceptionCore/PerceptionCore.h"
#include "../SurvivalCore/SurvivalCore.h"
#include "../HunterCore/RunRules.h"
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace thug::campaign {

// All values are playtest hypotheses. Changing them requires a new revision.
inline constexpr const char* CombatRuntimeRevision = "combat-actors-2";
struct CombatTuning {
  int magazineRounds = 30;
  double reloadSeconds = 2.5, throwSeconds = .5, shotInterval = .12;
  double shotLifetime = 2, grenadeFuse = 3, smokeFuse = 1;
  double blastRadius = 5, smokeRadius = 4, smokeSeconds = 20;
  double shotInterrupt = .6, blastInterrupt = 1.5, interruptImmunity = 4;
  double oxygenCapacity = 100, oxygenDrain = 6, oxygenRecovery = 10;
  double holdThreshold = 50, gaspSeconds = 3;
};
CombatTuning CombatProfile(rules::Difficulty);

enum class CombatOp {
  BeginReload, CompleteReload, CancelReload, Fire, ResolveShot,
  BeginThrow, ReleaseThrow, CancelThrow, Detonate, HoldBreath, ReleaseBreath
};
struct BlastContact {
  std::string target, node;
  hunter::Vec3 position;
  // Authored by the physical damage adapter after distance/occlusion checks.
  // Hunter and objects require None: Hunter has no combat health pool.
  survival::Injury injury = survival::Injury::None;
};
struct CombatCommand {
  std::string eventId;
  CombatOp kind = CombatOp::Fire;
  std::string node, target;
  double atSeconds = 0;
  std::uint64_t serial = 0;
  hunter::Vec3 position{}, vector{}; // Muzzle/direction or throw position/velocity, meters.
  std::vector<BlastContact> contacts{}; // Verified complete blast query, never inferred from room occupancy.
  std::string actor = "joe";
};
struct CombatContext {
  std::string actualJoeNode;
  bool physicallyVerified = false;
  bool animationCompleted = false;
  bool collisionVerified = false;
  std::map<std::string, std::string> actualActorNodes{};
  bool hunterPerceived = false, distractionIntended = false;
};
struct CombatTask {
  std::uint64_t serial = 0;
  CombatOp kind = CombatOp::BeginReload;
  std::string item, startEventId, node;
  double startedAt = 0, readyAt = 0;
};
struct Shot {
  std::uint64_t serial = 0;
  std::string eventId, node;
  double firedAt = 0, expiresAt = 0;
  hunter::Vec3 origin{}, direction{};
  std::string actor = "joe";
};
struct ThrownItem {
  std::uint64_t serial = 0;
  std::string eventId, item, node;
  double releasedAt = 0, detonateAt = 0;
  hunter::Vec3 position{}, velocity{};
  std::string actor = "joe";
};
struct SmokeCloud {
  std::uint64_t serial = 0;
  std::string eventId, node;
  double startedAt = 0, expiresAt = 0;
  hunter::Vec3 center{};
};
struct ActorCombatState {
  int loadedRounds = 0;
  double lastShotAt = -1;
  std::optional<CombatTask> task;
};
struct CombatRuntimeSnapshot {
  int loadedRounds = 0; // Initial magazines remain in inventory until the first completed reload.
  std::uint64_t serial = 0;
  std::uint64_t commandSerial = 0;
  double lastShotAt = -1, hunterBlockedUntil = 0, hunterImmuneUntil = 0;
  double oxygen = 100, gaspUntil = 0;
  bool holdingBreath = false;
  std::optional<CombatTask> task{};
  std::map<std::uint64_t, Shot> shots;
  std::map<std::uint64_t, ThrownItem> thrown;
  std::map<std::uint64_t, SmokeCloud> smoke;
  std::map<std::string, ActorCombatState> companions;
};
struct BreathingEffects {
  double oxygen = 0, capacity = 0;
  bool holding = false, gasping = false;
  double energy = 0; // [0,1], includes injury/fatigue; zero while holding.
};

} // namespace thug::campaign
