#pragma once

#include "../HunterCore/HunterCore.h"

#include <cstdint>
#include <map>
#include <optional>
#include <random>
#include <string>

namespace thug::perception {

enum class SoundKind {
  SlowFootstep, WalkFootstep, SprintFootstep, Door,
  BrokenObject, Fall, Gunshot, Explosion, Breathing, Distraction
};

struct SoundEvent {
  SoundKind kind;
  hunter::Vec3 actualSource; // Private to perception; never passed as a target.
  double atSeconds;
  double energy = 1; // [0, 1], e.g. affected by material or player condition.
};

struct ListenerContext {
  hunter::Vec3 location;
  double transmission = 1; // [0, 1], supplied after world occlusion checks.
  double masking = 0; // [0, 1], local ambient sound masking.
};

struct SoundProfile {
  double baseIntensity;
  double falloffMeters;
};

struct Tuning {
  double minimumEstimateRadiusMeters = 4;
  double minimumAudibleStrength = 0.02;
  double distanceUncertaintyFactor = 0.2;
  double occlusionUncertaintyMeters = 8;
  double maskingUncertaintyMeters = 6;
};

struct PerceptionSnapshot {
  Tuning tuning;
  std::string randomState;
};

struct EvidenceMemorySnapshot {
  std::map<std::string, std::string> lastSeen;
  double nowSeconds = 0;
};

SoundProfile ProfileFor(SoundKind kind);
std::string SoundName(SoundKind kind);
// Pass SurvivalCore::CurrentEffects().breathingLoudnessMultiplier, in [1, 3].
// Keeps normalized energy while retaining amplification above calm breathing.
SoundEvent MakeBreathingEvent(hunter::Vec3 source, double atSeconds,
                             double loudnessMultiplier);

class PerceptionCore {
public:
  explicit PerceptionCore(Tuning tuning = {}, std::uint64_t seed = 1);

  // The output hides actor identity and exact coordinates, including decoys.
  // May return no event when the sound is too faint or fully occluded.
  std::optional<hunter::Stimulus> Hear(const SoundEvent& sound,
                                       const ListenerContext& listener);
  PerceptionSnapshot CaptureSnapshot() const;
  bool RestoreSnapshot(const PerceptionSnapshot& snapshot);

private:
  Tuning tuning_;
  std::mt19937_64 random_;
};

// Stores only what the Hunter has actually inspected, never world-global state.
class EvidenceMemory {
public:
  // Caller guarantees line of sight to the object at inspection time.
  // First inspection establishes a baseline; only a later change raises evidence.
  std::optional<hunter::Stimulus> Inspect(
      const std::string& objectId, const std::string& visibleState,
      hunter::Vec3 observedPosition, double atSeconds, double confidence = 1);
  EvidenceMemorySnapshot CaptureSnapshot() const;
  bool RestoreSnapshot(const EvidenceMemorySnapshot& snapshot);

private:
  std::map<std::string, std::string> lastSeen_;
  double nowSeconds_ = 0;
};

} // namespace thug::perception
