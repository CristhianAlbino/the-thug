#pragma once

namespace thug::survival {

enum class Injury { None, Light, Moderate, Severe };
enum class Activity { Rest, Walk, Sneak, Sprint };

// All numbers are balance hypotheses until playtested in Unreal.
struct Tuning {
  double maxStamina = 100;
  double sprintDrainPerSecond = 18;
  double restRecoveryPerSecond = 12;
  double walkRecoveryFraction = 0.5;
  double moderateDrainMultiplier = 1.5;
  double severeDrainMultiplier = 2.2;
  double moderateRecoveryMultiplier = 0.75;
  double severeRecoveryMultiplier = 0.5;
  double exhaustionReleaseFraction = 0.25;
  double bleedingPerSecond = 0.35;
  double collapseBloodLoss = 70;
  double fatalBloodLoss = 100;
  double treatmentBloodRecovery = 20;
};

struct Effects {
  double speedMultiplier = 1;
  double breathingLoudnessMultiplier = 1;
  bool canSprint = true;
  bool collapsed = false;
  bool dead = false;
};

struct AdvanceResult {
  double sprintSeconds = 0;
  bool becameCollapsed = false;
  bool becameDead = false;
};

class SurvivalCore {
public:
  explicit SurvivalCore(Tuning tuning = {});

  // Damage mapping is authored by the world; injury can only worsen here.
  bool ApplyInjury(Injury severity);
  // Stabilization stops bleeding; treatment improves only one injury level.
  bool Stabilize();
  bool Treat();
  // Advances game time. The caller decides what Joe attempted to do.
  AdvanceResult Advance(double seconds, Activity requestedActivity);

  Injury CurrentInjury() const { return injury_; }
  double Stamina() const { return stamina_; }
  double BloodLoss() const { return bloodLoss_; }
  bool IsBleeding() const { return bleeding_; }
  bool IsExhausted() const { return exhausted_; }
  Effects CurrentEffects() const;

private:
  double DrainMultiplier() const;
  double RecoveryMultiplier() const;
  void Recover(double seconds, Activity activity);
  Tuning tuning_;
  Injury injury_ = Injury::None;
  double stamina_;
  double bloodLoss_ = 0;
  bool bleeding_ = false;
  bool exhausted_ = false;
  bool collapsed_ = false;
  bool dead_ = false;
};

} // namespace thug::survival
