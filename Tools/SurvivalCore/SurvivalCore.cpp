#include "SurvivalCore.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace thug::survival {
namespace {

bool Positive(double value) { return std::isfinite(value) && value > 0; }
bool NonNegative(double value) { return std::isfinite(value) && value >= 0; }

bool SameTuning(const Tuning& a, const Tuning& b) {
  return a.maxStamina == b.maxStamina &&
         a.sprintDrainPerSecond == b.sprintDrainPerSecond &&
         a.restRecoveryPerSecond == b.restRecoveryPerSecond &&
         a.walkRecoveryFraction == b.walkRecoveryFraction &&
         a.moderateDrainMultiplier == b.moderateDrainMultiplier &&
         a.severeDrainMultiplier == b.severeDrainMultiplier &&
         a.moderateRecoveryMultiplier == b.moderateRecoveryMultiplier &&
         a.severeRecoveryMultiplier == b.severeRecoveryMultiplier &&
         a.exhaustionReleaseFraction == b.exhaustionReleaseFraction &&
         a.bleedingPerSecond == b.bleedingPerSecond &&
         a.collapseBloodLoss == b.collapseBloodLoss &&
         a.fatalBloodLoss == b.fatalBloodLoss &&
         a.treatmentBloodRecovery == b.treatmentBloodRecovery;
}

} // namespace

SurvivalCore::SurvivalCore(Tuning tuning)
    : tuning_(tuning), stamina_(tuning.maxStamina) {
  if (!Positive(tuning.maxStamina) ||
      !Positive(tuning.sprintDrainPerSecond) ||
      !Positive(tuning.restRecoveryPerSecond) ||
      !NonNegative(tuning.walkRecoveryFraction) ||
      tuning.walkRecoveryFraction > 1 ||
      !Positive(tuning.moderateDrainMultiplier) ||
      !Positive(tuning.severeDrainMultiplier) ||
      !Positive(tuning.moderateRecoveryMultiplier) ||
      !Positive(tuning.severeRecoveryMultiplier) ||
      !Positive(tuning.exhaustionReleaseFraction) ||
      tuning.exhaustionReleaseFraction > 1 ||
      !Positive(tuning.bleedingPerSecond) ||
      !Positive(tuning.collapseBloodLoss) ||
      !Positive(tuning.fatalBloodLoss) ||
      tuning.collapseBloodLoss >= tuning.fatalBloodLoss ||
      !NonNegative(tuning.treatmentBloodRecovery)) {
    throw std::invalid_argument("Invalid survival tuning");
  }
}

bool SurvivalCore::ApplyInjury(Injury severity) {
  if (dead_ || severity == Injury::None ||
      static_cast<int>(severity) < static_cast<int>(injury_) ||
      static_cast<int>(severity) > static_cast<int>(Injury::Severe)) {
    return false;
  }
  const bool worsened = static_cast<int>(severity) > static_cast<int>(injury_);
  const bool reopened = severity == Injury::Severe && !bleeding_;
  if (!worsened && !reopened) return false;
  injury_ = severity;
  if (severity == Injury::Severe) bleeding_ = true;
  return true;
}

bool SurvivalCore::Stabilize() {
  if (dead_ || !bleeding_) return false;
  bleeding_ = false;
  return true;
}

bool SurvivalCore::Treat() {
  if (dead_ || (injury_ == Injury::None && bloodLoss_ == 0 &&
                 !bleeding_)) return false;
  if (injury_ != Injury::None) {
    injury_ = static_cast<Injury>(static_cast<int>(injury_) - 1);
  }
  bloodLoss_ = std::max(0.0, bloodLoss_ - tuning_.treatmentBloodRecovery);
  bleeding_ = false;
  if (bloodLoss_ < tuning_.collapseBloodLoss) collapsed_ = false;
  return true;
}

double SurvivalCore::DrainMultiplier() const {
  switch (injury_) {
  case Injury::Moderate: return tuning_.moderateDrainMultiplier;
  case Injury::Severe: return tuning_.severeDrainMultiplier;
  default: return 1;
  }
}

double SurvivalCore::RecoveryMultiplier() const {
  switch (injury_) {
  case Injury::Moderate: return tuning_.moderateRecoveryMultiplier;
  case Injury::Severe: return tuning_.severeRecoveryMultiplier;
  default: return 1;
  }
}

void SurvivalCore::Recover(double seconds, Activity activity) {
  const double movementFraction =
      activity == Activity::Rest ? 1 : tuning_.walkRecoveryFraction;
  stamina_ = std::min(tuning_.maxStamina,
                      stamina_ + seconds * tuning_.restRecoveryPerSecond *
                                     RecoveryMultiplier() * movementFraction);
  if (exhausted_ && stamina_ >=
                        tuning_.maxStamina * tuning_.exhaustionReleaseFraction) {
    exhausted_ = false;
  }
}

AdvanceResult SurvivalCore::Advance(double seconds, Activity requestedActivity) {
  if (!NonNegative(seconds)) {
    throw std::invalid_argument("Elapsed time must be finite and non-negative");
  }
  if (requestedActivity != Activity::Rest &&
      requestedActivity != Activity::Walk &&
      requestedActivity != Activity::Sneak &&
      requestedActivity != Activity::Sprint) {
    throw std::invalid_argument("Invalid activity");
  }
  AdvanceResult result;
  if (dead_ || seconds == 0) return result;

  double mobileSeconds = collapsed_ ? 0 : seconds;
  if (bleeding_ && !collapsed_) {
    mobileSeconds = std::min(mobileSeconds,
        std::max(0.0, (tuning_.collapseBloodLoss - bloodLoss_) /
                          tuning_.bleedingPerSecond));
  }

  if (mobileSeconds > 0) {
    if (requestedActivity == Activity::Sprint && CurrentEffects().canSprint) {
      const double drain = tuning_.sprintDrainPerSecond * DrainMultiplier();
      result.sprintSeconds = std::min(mobileSeconds, stamina_ / drain);
      stamina_ = std::max(0.0, stamina_ - result.sprintSeconds * drain);
      if (stamina_ <= 0.000001) {
        stamina_ = 0;
        exhausted_ = true;
      }
      Recover(mobileSeconds - result.sprintSeconds, Activity::Walk);
    } else {
      Recover(mobileSeconds,
              requestedActivity == Activity::Sprint ? Activity::Walk
                                                    : requestedActivity);
    }
  }

  if (bleeding_) {
    bloodLoss_ = std::min(tuning_.fatalBloodLoss,
                          bloodLoss_ + seconds * tuning_.bleedingPerSecond);
    if (!collapsed_ && bloodLoss_ >= tuning_.collapseBloodLoss) {
      collapsed_ = true;
      result.becameCollapsed = true;
    }
    if (bloodLoss_ >= tuning_.fatalBloodLoss) {
      dead_ = true;
      result.becameDead = true;
    }
  }
  return result;
}

Effects SurvivalCore::CurrentEffects() const {
  Effects effects;
  switch (injury_) {
  case Injury::Light:
    effects.speedMultiplier = 0.95;
    effects.breathingLoudnessMultiplier = 1.2;
    break;
  case Injury::Moderate:
    effects.speedMultiplier = 0.8;
    effects.breathingLoudnessMultiplier = 1.6;
    break;
  case Injury::Severe:
    effects.speedMultiplier = 0.65;
    effects.breathingLoudnessMultiplier = 2;
    break;
  case Injury::None:
    break;
  }
  if (stamina_ <= tuning_.maxStamina * 0.25) {
    effects.speedMultiplier *= 0.8;
    effects.breathingLoudnessMultiplier *= 1.5;
  }
  effects.collapsed = collapsed_;
  effects.dead = dead_;
  effects.canSprint = !exhausted_ && stamina_ > 0 &&
                      !collapsed_ && !dead_;
  if (collapsed_ || dead_) effects.speedMultiplier = 0;
  return effects;
}

SurvivalSnapshot SurvivalCore::CaptureSnapshot() const {
  return {tuning_, injury_, stamina_, bloodLoss_, bleeding_, exhausted_,
          collapsed_, dead_};
}

bool SurvivalCore::RestoreSnapshot(const SurvivalSnapshot& s) {
  if (!SameTuning(s.tuning, tuning_) ||
      s.injury < Injury::None || s.injury > Injury::Severe ||
      !NonNegative(s.stamina) || s.stamina > tuning_.maxStamina ||
      !NonNegative(s.bloodLoss) || s.bloodLoss > tuning_.fatalBloodLoss ||
      (s.bleeding && s.injury != Injury::Severe) ||
      (s.collapsed && s.bloodLoss < tuning_.collapseBloodLoss) ||
      (!s.collapsed && s.bloodLoss >= tuning_.collapseBloodLoss) ||
      (!s.dead && s.bloodLoss >= tuning_.fatalBloodLoss) ||
      (!s.exhausted && s.stamina == 0) ||
      (s.exhausted && s.stamina >=
          tuning_.maxStamina * tuning_.exhaustionReleaseFraction) ||
      (s.dead && (!s.collapsed || !s.bleeding ||
                  s.bloodLoss < tuning_.fatalBloodLoss))) return false;
  injury_ = s.injury;
  stamina_ = s.stamina;
  bloodLoss_ = s.bloodLoss;
  bleeding_ = s.bleeding;
  exhausted_ = s.exhausted;
  collapsed_ = s.collapsed;
  dead_ = s.dead;
  return true;
}

} // namespace thug::survival
