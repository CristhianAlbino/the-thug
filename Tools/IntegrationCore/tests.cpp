#include "IntegrationCore.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace thug;

namespace {

int checks = 0;

void Check(bool value, const char* reason) {
  ++checks;
  if (!value) throw std::runtime_error(reason);
}

std::vector<survival::ItemDefinition> Catalog() {
  return {{"magazine", survival::ItemCategory::Ammunition, 1, 5},
          {"bandage", survival::ItemCategory::Medical, 1, 5}};
}

companion::Traits Neutral(int ammo = 0) {
  return {50, 50, 50, 50, 50, 50, ammo, false};
}

void SuccessfulShareAndReplay() {
  survival::Inventory joe(5, Catalog());
  companion::CompanionCore harley(companion::Character::Harley,
                                  companion::Difficulty::Normal, Neutral());
  Check(joe.Add("magazine", 2), "Joe starts with two units");
  Check(integration::ShareAmmunition(joe, "magazine", harley,
                                     "handoff-harley-1", 1),
        "Confirmed handoff succeeds");
  Check(joe.Quantity("magazine") == 1 &&
            harley.CurrentTraits().ammunition == 1,
        "One unit moved, without duplication");
  Check(harley.CurrentTraits().trust == 70 &&
            harley.CurrentTraits().loyalty == 60,
        "Exact GDD reaction recorded");
  Check(!integration::ShareAmmunition(joe, "magazine", harley,
                                      "handoff-harley-1", 2),
        "Replay of same persistent event is rejected");
  Check(joe.Quantity("magazine") == 1 &&
            harley.CurrentTraits().ammunition == 1 &&
            harley.CurrentTraits().trust == 70,
        "Replay changed neither stock nor relationship");
  Check(integration::ShareAmmunition(joe, "magazine", harley,
                                     "handoff-harley-2", 2),
        "A distinct real handoff succeeds");
  Check(joe.Quantity("magazine") == 0 &&
            harley.CurrentTraits().ammunition == 2,
        "Two real handoffs conserve both units");
  Check(!integration::ShareAmmunition(joe, "magazine", harley,
                                      "handoff-harley-3", 3),
        "Cannot grant ammo or trust when Joe has no stock");
  Check(harley.CurrentTraits().ammunition == 2 &&
            harley.CurrentTraits().trust == 90,
        "Out-of-stock attempt did not mutate companion");
}

void RejectInvalidHandoffs() {
  survival::Inventory joe(5, Catalog());
  companion::CompanionCore jack(companion::Character::Jack,
                                companion::Difficulty::Hard, Neutral());
  Check(joe.Add("magazine", 1) && joe.Add("bandage", 1),
        "Test items available");
  Check(!integration::ShareAmmunition(joe, "bandage", jack, "wrong-item", 1),
        "Medical item cannot be passed as ammunition");
  Check(!integration::ShareAmmunition(joe, "missing", jack, "unknown", 1),
        "Unknown item rejected");
  Check(!integration::ShareAmmunition(joe, "magazine", jack, "", 1),
        "Missing event ID rejected");
  Check(!integration::ShareAmmunition(
            joe, "magazine", jack, "nan", std::numeric_limits<double>::quiet_NaN()),
        "Invalid time rejected without exception");
  Check(joe.Quantity("magazine") == 1 && joe.Quantity("bandage") == 1 &&
            jack.CurrentTraits().ammunition == 0 &&
            jack.CurrentTraits().trust == 50,
        "Invalid handoffs changed neither participant");
  Check(integration::ShareAmmunition(joe, "magazine", jack, "good", 10),
        "First valid event still succeeds");
  Check(joe.Add("magazine", 1), "Restore one unit for time check");
  Check(!integration::ShareAmmunition(joe, "magazine", jack, "past", 9),
        "Out-of-order event rejected");
  Check(joe.Quantity("magazine") == 1 &&
            jack.CurrentTraits().ammunition == 1 &&
            jack.CurrentTraits().trust == 70,
        "Out-of-order attempt changed neither participant");
}

void RejectFullCompanionResource() {
  survival::Inventory joe(1, Catalog());
  companion::CompanionCore barth(
      companion::Character::Bartholomeu, companion::Difficulty::Normal,
      Neutral(std::numeric_limits<int>::max()));
  Check(joe.Add("magazine", 1), "Joe has one unit");
  Check(!integration::ShareAmmunition(joe, "magazine", barth, "overflow", 1),
        "Resource overflow rejected");
  Check(joe.Quantity("magazine") == 1 &&
            barth.CurrentTraits().ammunition == std::numeric_limits<int>::max() &&
            barth.CurrentTraits().trust == 50,
        "Overflow did not spend or grant trust");
}

} // namespace

int main() {
  try {
    SuccessfulShareAndReplay();
    RejectInvalidHandoffs();
    RejectFullCompanionResource();
    std::cout << "IntegrationCore: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "IntegrationCore test failed after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
