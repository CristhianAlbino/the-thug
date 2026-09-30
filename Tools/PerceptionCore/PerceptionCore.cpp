#include "PerceptionCore.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace thug::perception {
namespace {

bool ValidPosition(const hunter::Vec3& position) {
  return std::isfinite(position.x) && std::isfinite(position.y) &&
         std::isfinite(position.z);
}

double Distance(const hunter::Vec3& a, const hunter::Vec3& b) {
  return std::hypot(std::hypot(a.x - b.x, a.y - b.y), a.z - b.z);
}

bool SameTuning(const Tuning& a, const Tuning& b) {
  return a.minimumEstimateRadiusMeters == b.minimumEstimateRadiusMeters &&
         a.minimumAudibleStrength == b.minimumAudibleStrength &&
         a.distanceUncertaintyFactor == b.distanceUncertaintyFactor &&
         a.occlusionUncertaintyMeters == b.occlusionUncertaintyMeters &&
         a.maskingUncertaintyMeters == b.maskingUncertaintyMeters;
}

} // namespace

SoundProfile ProfileFor(SoundKind kind) {
  // Initial balance hypotheses; GDD specifies relative, not numerical loudness.
  switch (kind) {
  case SoundKind::SlowFootstep: return {0.18, 12};
  case SoundKind::WalkFootstep: return {0.25, 16};
  case SoundKind::SprintFootstep: return {0.55, 25};
  case SoundKind::Door: return {0.5, 30};
  case SoundKind::BrokenObject: return {0.75, 40};
  case SoundKind::Fall: return {0.85, 45};
  case SoundKind::Gunshot: return {1, 100};
  case SoundKind::Explosion: return {1, 150};
  case SoundKind::Breathing: return {0.12, 8};
  case SoundKind::Distraction: return {0.65, 35};
  }
  throw std::invalid_argument("Unknown sound kind");
}

std::string SoundName(SoundKind kind) {
  switch (kind) {
  case SoundKind::SlowFootstep: return "slow_footstep";
  case SoundKind::WalkFootstep: return "walk_footstep";
  case SoundKind::SprintFootstep: return "sprint_footstep";
  case SoundKind::Door: return "door";
  case SoundKind::BrokenObject: return "broken_object";
  case SoundKind::Fall: return "fall";
  case SoundKind::Gunshot: return "gunshot";
  case SoundKind::Explosion: return "explosion";
  case SoundKind::Breathing: return "breathing";
  case SoundKind::Distraction: return "distraction";
  }
  throw std::invalid_argument("Unknown sound kind");
}

PerceptionCore::PerceptionCore(Tuning tuning, std::uint64_t seed)
    : tuning_(tuning), random_(seed) {
  if (!std::isfinite(tuning_.minimumEstimateRadiusMeters) ||
      tuning_.minimumEstimateRadiusMeters < 4 ||
      !std::isfinite(tuning_.minimumAudibleStrength) ||
      tuning_.minimumAudibleStrength <= 0 ||
      tuning_.minimumAudibleStrength > 1 ||
      !std::isfinite(tuning_.distanceUncertaintyFactor) ||
      tuning_.distanceUncertaintyFactor < 0 ||
      !std::isfinite(tuning_.occlusionUncertaintyMeters) ||
      tuning_.occlusionUncertaintyMeters < 0 ||
      !std::isfinite(tuning_.maskingUncertaintyMeters) ||
      tuning_.maskingUncertaintyMeters < 0) {
    throw std::invalid_argument("Invalid perception tuning");
  }
}

std::optional<hunter::Stimulus> PerceptionCore::Hear(
    const SoundEvent& sound, const ListenerContext& listener) {
  if (!ValidPosition(sound.actualSource) || !ValidPosition(listener.location) ||
      !std::isfinite(sound.atSeconds) || sound.atSeconds < 0 ||
      !std::isfinite(sound.energy) || sound.energy < 0 || sound.energy > 1 ||
      !std::isfinite(listener.transmission) || listener.transmission < 0 ||
      listener.transmission > 1 || !std::isfinite(listener.masking) ||
      listener.masking < 0 || listener.masking > 1) {
    throw std::invalid_argument("Invalid sound or listener context");
  }
  const auto profile = ProfileFor(sound.kind);
  const double distance = Distance(sound.actualSource, listener.location);
  const double relative = distance / profile.falloffMeters;
  const double heardStrength =
      profile.baseIntensity * sound.energy * listener.transmission *
      (1 - listener.masking) / (1 + relative * relative);
  if (heardStrength < tuning_.minimumAudibleStrength) return std::nullopt;

  const double radius = std::max(
      tuning_.minimumEstimateRadiusMeters,
      tuning_.minimumEstimateRadiusMeters +
          distance * tuning_.distanceUncertaintyFactor +
          (1 - listener.transmission) *
              tuning_.occlusionUncertaintyMeters +
          listener.masking * tuning_.maskingUncertaintyMeters);
  // Keep the perceived center inside the uncertainty area but away from the
  // authoritative source. A listener can investigate the wrong corner.
  std::uniform_real_distribution<double> angle(0, 6.283185307179586);
  std::uniform_real_distribution<double> radial(0.25, 0.75);
  const double a = angle(random_);
  const double offset = radius * radial(random_);
  const hunter::Vec3 center{sound.actualSource.x + std::cos(a) * offset,
                            sound.actualSource.y + std::sin(a) * offset,
                            sound.actualSource.z};
  return hunter::Stimulus{
      hunter::StimulusKind::Sound, hunter::Subject::Unknown, sound.atSeconds,
      hunter::LocationEstimate{center, radius, sound.atSeconds},
      std::clamp(heardStrength, 0.0, 1.0), "", SoundName(sound.kind)};
}

PerceptionSnapshot PerceptionCore::CaptureSnapshot() const {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << random_;
  return {tuning_, out.str()};
}

bool PerceptionCore::RestoreSnapshot(const PerceptionSnapshot& s) {
  if (!SameTuning(s.tuning, tuning_) || s.randomState.empty()) return false;
  std::istringstream in(s.randomState);
  in.imbue(std::locale::classic());
  std::mt19937_64 random;
  if (!(in >> random)) return false;
  in >> std::ws;
  if (!in.eof()) return false;
  random_ = random;
  return true;
}

std::optional<hunter::Stimulus> EvidenceMemory::Inspect(
    const std::string& objectId, const std::string& visibleState,
    hunter::Vec3 observedPosition, double atSeconds, double confidence) {
  if (objectId.empty() || visibleState.empty() ||
      !ValidPosition(observedPosition) || !std::isfinite(atSeconds) ||
      atSeconds < nowSeconds_ || !std::isfinite(confidence) ||
      confidence <= 0 || confidence > 1) {
    throw std::invalid_argument("Invalid evidence inspection");
  }
  nowSeconds_ = atSeconds;
  const auto [it, inserted] = lastSeen_.emplace(objectId, visibleState);
  if (inserted || it->second == visibleState) return std::nullopt;
  it->second = visibleState;
  return hunter::Stimulus{
      hunter::StimulusKind::EnvironmentalEvidence, hunter::Subject::Unknown,
      atSeconds, hunter::LocationEstimate{observedPosition, 4, atSeconds},
      confidence, "", objectId + ":" + visibleState};
}

EvidenceMemorySnapshot EvidenceMemory::CaptureSnapshot() const {
  return {lastSeen_, nowSeconds_};
}

bool EvidenceMemory::RestoreSnapshot(const EvidenceMemorySnapshot& s) {
  if (!std::isfinite(s.nowSeconds) || s.nowSeconds < 0) return false;
  for (const auto& [id, visibleState] : s.lastSeen)
    if (id.empty() || visibleState.empty()) return false;
  auto states = s.lastSeen;
  lastSeen_.swap(states);
  nowSeconds_ = s.nowSeconds;
  return true;
}

} // namespace thug::perception
