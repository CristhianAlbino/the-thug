#pragma once
#include "../DirectorCore/DirectorCore.h"
#include <optional>

namespace thug::campaign {
inline constexpr const char* SpecialRuntimeRevision = "rare-finales-1";
enum class SpecialOp {
  Recruit, StartConfrontation, ResolveConfrontation, ConfrontationEscape,
  AcquireSupply, BeginPlant, CompletePlant, CancelPlant,
  DetonateCollapse, ResolveCollapse, WitnessAppearance
};
enum class SpecialPhase { Idle, Confronting, Opportunity, Failed, Escaped, Collapsing, Destroyed, FatalCollapse };
enum class SpecialFate { Injured, Captured, Dead, Escaped };
struct SpecialCommand {
  std::string eventId;
  SpecialOp kind = SpecialOp::Recruit;
  std::string target, node;
  double atSeconds = 0;
  std::uint64_t serial = 0;
  int argument = 0; // ResolveConfrontation: interruption 0/1; ResolveCollapse: Joe survived 0/1.
  std::map<std::string, SpecialFate> fates;
};
struct SpecialContext {
  std::string actualJoeNode, actualHunterNode;
  std::map<std::string, director::Actor> participants;
  bool physicallyVerified = false, actionCompleted = false;
  bool movementCompleted = false, warningPresented = false;
  bool hunterIncapacitatedVerified = false, joeSurvivedCollapse = false;
  bool joeDeathVerified = false;
  // Each listed fate was physically confirmed AND witnessed by Joe.
  bool fatesWitnessed = false, appearancePerceived = false;
};
struct PlantTask {
  std::uint64_t serial = 0;
  std::string site, eventId;
  double startedAt = 0, readyAt = 0;
};
struct SpecialRuntimeSnapshot {
  std::uint64_t commandSerial = 0, plantSerial = 0;
  unsigned appearanceTicket = 0; // One campaign draw, not rerolled on query/load.
  SpecialPhase phase = SpecialPhase::Idle;
  double boundaryAt = -1;
  std::set<std::string> recruited, supplies, planted, appearances;
  std::optional<PlantTask> plant;
};
} // namespace thug::campaign
