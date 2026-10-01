#include "Inventory.h"
#include "SurvivalCore.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace thug::survival;

namespace {

int checks = 0;

void Check(bool value, const char* reason) {
  ++checks;
  if (!value) throw std::runtime_error(reason);
}

void StaminaAndInjury() {
  SurvivalCore joe;
  Check(joe.ApplyInjury(Injury::Light), "Light injury accepted");
  Check(joe.CurrentEffects().speedMultiplier < 1 &&
            joe.CurrentEffects().breathingLoudnessMultiplier > 1,
        "Light injury affects movement and audio");
  Check(joe.ApplyInjury(Injury::Moderate), "Worsening injury accepted");
  Check(joe.CurrentEffects().canSprint, "Moderate injury limits sprint");
  auto result = joe.Advance(4, Activity::Sprint);
  Check(result.sprintSeconds > 0 && result.sprintSeconds < 4,
        "Large time step stops sprint on exhaustion");
  Check(!joe.CurrentEffects().canSprint, "Exhaustion gates further sprint");
  Check(!joe.CurrentEffects().dead && joe.Stamina() >= 0,
        "Stamina exhaustion does not kill Joe");
  joe.Advance(10, Activity::Rest);
  Check(joe.CurrentEffects().canSprint,
        "Recovery releases exhaustion after threshold");
  Check(joe.ApplyInjury(Injury::Severe), "Severe injury accepted");
  Check(joe.IsBleeding(), "Severe injury starts bleeding");
  Check(joe.Stabilize(), "Bleeding can be stopped");
  Check(!joe.IsBleeding(), "Stabilization stops loss");
  Check(joe.ApplyInjury(Injury::Severe),
        "Another severe injury can reopen bleeding");
  Check(joe.Treat(), "Medical treatment accepted");
  Check(joe.CurrentInjury() == Injury::Moderate && !joe.IsBleeding(),
        "One dose improves one level, never resets everything");
}

void BleedingAndRecovery() {
  Tuning fast;
  fast.bleedingPerSecond = 10;
  fast.collapseBloodLoss = 50;
  fast.fatalBloodLoss = 100;
  fast.treatmentBloodRecovery = 30;
  SurvivalCore joe(fast);
  joe.ApplyInjury(Injury::Severe);
  auto result = joe.Advance(6, Activity::Sprint);
  Check(result.becameCollapsed && !result.becameDead,
        "Blood loss first causes collapse");
  Check(result.sprintSeconds <= 5 && joe.CurrentEffects().speedMultiplier == 0,
        "No movement after collapse within a large step");
  Check(joe.BloodLoss() == 60, "Blood loss accumulated over game time");
  Check(joe.Stabilize(), "Can stop bleeding after collapse");
  joe.Advance(100, Activity::Rest);
  Check(joe.BloodLoss() == 60 && !joe.CurrentEffects().dead,
        "Stabilization prevents further loss");
  Check(joe.Treat(), "Treatment after stabilization");
  Check(!joe.CurrentEffects().collapsed && joe.BloodLoss() == 30,
        "Partial treatment permits mobility below threshold");
  joe.ApplyInjury(Injury::Severe);
  result = joe.Advance(7, Activity::Rest);
  Check(result.becameDead && joe.CurrentEffects().dead,
        "Untreated severe loss can eventually kill");
  Check(!joe.Treat() && !joe.Stabilize(),
        "Medicine cannot revive after death");
}

void RejectInvalidInputs() {
  Tuning invalid;
  invalid.collapseBloodLoss = invalid.fatalBloodLoss;
  bool threw = false;
  try { SurvivalCore impossible(invalid); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Invalid tuning rejected");
  SurvivalCore joe;
  threw = false;
  try { joe.Advance(-1, Activity::Rest); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw && joe.Stamina() == 100, "Negative time has no effect");
  Check(!joe.ApplyInjury(Injury::None), "No-op injury rejected");
  Check(joe.ApplyInjury(Injury::Moderate), "Moderate applied");
  Check(!joe.ApplyInjury(Injury::Light), "Damage cannot heal injury");
  const auto before = joe.CaptureSnapshot();
  auto broken = before;
  broken.injury = Injury::Severe;
  broken.bloodLoss = broken.tuning.fatalBloodLoss;
  broken.collapsed = true;
  broken.bleeding = true;
  Check(!joe.RestoreSnapshot(broken), "Fatal blood loss cannot restore a living Joe");
  broken = before;
  broken.stamina = 0;
  Check(!joe.RestoreSnapshot(broken), "Empty stamina must retain exhaustion");
  broken = before;
  broken.exhausted = true;
  Check(!joe.RestoreSnapshot(broken), "Recovered stamina cannot remain exhausted");
  Check(joe.Stamina() == before.stamina && joe.BloodLoss() == before.bloodLoss &&
        joe.CurrentInjury() == before.injury, "Rejected snapshots leave survival untouched");
}

std::vector<ItemDefinition> Catalog() {
  return {{"rifle", ItemCategory::Weapon, 3, 1, false},
          {"magazine", ItemCategory::Ammunition, 1, 4, true},
          {"tool", ItemCategory::Utility, 2, 2, true},
          {"key", ItemCategory::Utility, 1, 1, false},
          {"medicine", ItemCategory::Medical, 1, 3, true}};
}

void CapacityAndProtectedItems() {
  Inventory joe(5, Catalog());
  Check(joe.Add("rifle", 1), "Rifle fits");
  Check(joe.Add("magazine", 2), "Two magazines fit exactly");
  Check(!joe.Add("tool", 1), "Tool competes for space with ammunition");
  Check(joe.UsedSlots() == 5, "Capacity enforced");
  Check(!joe.Add("unknown", 1) && !joe.Add("magazine", 0),
        "Unknown and zero quantity rejected");
  Check(joe.Remove("magazine", 2, RemovalReason::Drop),
        "Can free space by dropping ammunition");
  Check(joe.Add("tool", 1), "Alternative tool now fits");
  Check(!joe.Remove("rifle", 1, RemovalReason::Drop),
        "Protected item cannot be casually dropped");
  Check(joe.Quantity("rifle") == 1, "Rejected drop leaves item");

  Inventory quest(1, Catalog());
  Check(quest.Add("key", 1), "Quest key fits");
  Check(!quest.Remove("key", 1, RemovalReason::Drop),
        "Quest key is protected from drop");
  Check(quest.Remove("key", 1, RemovalReason::Consume),
        "Objective may consume quest key");
}

void TransferIsAllOrNothing() {
  Inventory joe(4, Catalog()), ally(1, Catalog());
  Check(joe.Add("magazine", 3), "Joe has magazines");
  Check(!joe.TransferTo(ally, "magazine", 2),
        "Insufficient companion space rejects entire transfer");
  Check(joe.Quantity("magazine") == 3 && ally.Quantity("magazine") == 0,
        "Neither inventory changed on failure");
  Check(joe.TransferTo(ally, "magazine", 1),
        "One magazine can be transferred");
  Check(joe.Quantity("magazine") == 2 && ally.Quantity("magazine") == 1,
        "Transfer conserves quantity");
  Check(!joe.TransferTo(joe, "magazine", 1),
        "Cannot transfer to self");
  Check(!joe.Remove("magazine", 99, RemovalReason::Consume),
        "Cannot consume more than held");
  bool threw = false;
  try { Inventory invalid(2, {{"x", ItemCategory::Utility},
                              {"x", ItemCategory::Utility}}); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Duplicate catalog IDs rejected");
}

} // namespace

int main() {
  try {
    StaminaAndInjury();
    BleedingAndRecovery();
    RejectInvalidInputs();
    CapacityAndProtectedItems();
    TransferIsAllOrNothing();
    std::cout << "SurvivalCore: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "SurvivalCore test failed after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
