#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace thug::hunter {

struct Vec3 {
  double x = 0;
  double y = 0;
  double z = 0;
};

// Coordinates are perceived positions, never authoritative player coordinates.
struct LocationEstimate {
  Vec3 center;
  double radiusMeters = 0;
  double observedAtSeconds = 0;
};

enum class Subject { Unknown, Player, Companion, Decoy };
enum class StimulusKind {
  Sound,
  VisualContact,
  VisualLost,
  EnvironmentalEvidence,
  ObservedRoute,
  ConfirmedHideSpot
};
enum class State { Patrol, Investigating, Pursuing, Searching };
enum class Difficulty { Easy, Normal, Hard, Punitive };

// These observations must be produced by an actual sensor or observed evidence.
// A sound's position is approximate even when its cause was the player.
struct Stimulus {
  Stimulus() = default;
  Stimulus(StimulusKind kind, Subject subject, double atSeconds,
           std::optional<LocationEstimate> estimate, double strength,
           std::string observedId, std::string perceivedType = {})
      : kind(kind), subject(subject), atSeconds(atSeconds),
        estimate(std::move(estimate)), strength(strength),
        observedId(std::move(observedId)),
        perceivedType(std::move(perceivedType)) {}
  StimulusKind kind = StimulusKind::Sound;
  Subject subject = Subject::Unknown;
  double atSeconds = 0;
  std::optional<LocationEstimate> estimate;
  double strength = 1; // In [0, 1], after occlusion and sensor attenuation.
  std::string observedId; // Route or hiding spot ID, as appropriate.
  std::string perceivedType; // Classified sound/evidence; empty means unknown.
};

struct Tuning {
  double minimumSoundRadiusMeters = 4;
  double soundThreshold = 0.08;
  double searchSeconds = 40;
  double investigateSeconds = 16;
  double memoryHalfLifeSeconds = 300;
  double locationLifetimeSeconds = 120;
  double minimumRouteWeight = 1;
  double routeEvidenceWeight = 2;
  double hideSuspicionCap = 90;
};

Tuning TuningFor(Difficulty difficulty);

struct Intent {
  State state = State::Patrol;
  std::optional<LocationEstimate> destination;
};

struct EvidenceSnapshot {
  double score = 0;
  double lastObservedAt = 0;
  int observations = 0;
};

struct HunterSnapshot {
  Tuning tuning;
  State state = State::Patrol;
  double nowSeconds = 0;
  double enteredStateAt = 0;
  std::optional<LocationEstimate> lastEstimate;
  std::string lastHeardType;
  std::map<std::string, EvidenceSnapshot> observedRoutes;
  std::map<std::string, EvidenceSnapshot> confirmedHideSpots;
  std::string randomState;
};

// Engine-agnostic decision core. Navigation, perception, animation, dialogue and
// damage remain the responsibility of the Unreal integration layer.
class HunterCore {
public:
  explicit HunterCore(Difficulty difficulty, std::uint64_t seed = 1);
  explicit HunterCore(Tuning tuning, std::uint64_t seed = 1);

  // Returns false when an observation is malformed, impossible or out of order.
  bool Observe(const Stimulus& stimulus);
  // Monotonic game time in seconds. Throws std::invalid_argument on invalid time.
  void AdvanceTo(double nowSeconds);
  Intent CurrentIntent() const;
  State CurrentState() const { return state_; }
  double HideSpotSuspicion(const std::string& id) const;
  double RouteEvidence(const std::string& id) const;
  const std::string& LastHeardType() const { return lastHeardType_; }
  // Picks among designer-provided valid routes. Never creates a route or moves an actor.
  std::string ChoosePatrolRoute(const std::vector<std::string>& availableRoutes);
  HunterSnapshot CaptureSnapshot() const;
  bool RestoreSnapshot(const HunterSnapshot& snapshot);

private:
  struct Evidence {
    double score = 0;
    double lastObservedAt = 0;
    int observations = 0;
  };

  double DecayedScore(const Evidence& value) const;
  bool Valid(const Stimulus& stimulus) const;
  void SetState(State state, double atSeconds);

  Tuning tuning_;
  std::mt19937_64 random_;
  State state_ = State::Patrol;
  double nowSeconds_ = 0;
  double enteredStateAt_ = 0;
  std::optional<LocationEstimate> lastEstimate_;
  std::string lastHeardType_;
  std::map<std::string, Evidence> observedRoutes_;
  std::map<std::string, Evidence> confirmedHideSpots_;
};

} // namespace thug::hunter
