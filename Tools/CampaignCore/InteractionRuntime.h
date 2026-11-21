#pragma once
#include "CompanionRuntime.h"
#include "../DirectorCore/DirectorCore.h"

namespace thug::campaign {
inline constexpr const char* InteractionRuntimeRevision = "party-interactions-1";
enum class InteractionOp { Signal, Gesture, PlaceWedge, RecoverWedge, Reunion, WitnessFate, Ambient, Banter, PerceiveThreat };
enum class SignalKind { Call, Quiet, PointArea };
enum class PersonalGesture { HarleyCalmsBarth, BarthHelpsJoe, JackHelpsJoe };
enum class WitnessedFate { Captured, Dead, LeftAlone, Together };
struct InteractionCommand {
  std::string eventId;
  InteractionOp kind = InteractionOp::Signal;
  std::string actor = "joe", target, node, destination, item;
  double atSeconds = 0;
  int argument = 0;
  bool physicallyVerified = false;
};
struct InteractionContext {
  // Only actors actually perceived by the executor; proximity is not LOS.
  std::map<std::string, director::Actor> actors;
  std::set<std::string> reachableNodes;
  bool signalPerceived = false, areaPerceived = false, actionCompleted = false,
       alternativePathVerified = false, inDanger = false, hunterPerceived = false;
  hunter::Vec3 actualSourceMeters{};
};
struct SignaledOrder {
  SignalKind kind = SignalKind::Call;
  std::string destination, eventId;
  double expiresAt = 0;
  bool fulfilled = false;
};
struct InstalledWedge { std::string item, eventId; double placedAt = 0; };
struct InteractionRuntimeSnapshot {
  unsigned variation = 0;
  std::uint64_t serial = 0;
  std::map<std::string, SignaledOrder> orders;
  std::map<std::string, double> quietUntil, threatUntil, gestureAt;
  std::map<std::string, InstalledWedge> wedges;
  std::set<std::string> ambientSeen, banterHeard, reunions;
  std::map<std::string, WitnessedFate> knownFates;
};
struct NightVariation { unsigned variant = 0; std::string id, node, ambient, speaker, line; };
struct NightRecordEntry { std::string eventId, actor, node, text; double atSeconds = 0; };
} // namespace thug::campaign
