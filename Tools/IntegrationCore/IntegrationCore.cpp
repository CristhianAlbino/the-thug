#include "IntegrationCore.h"

#include <stdexcept>

namespace thug::integration {

bool ShareAmmunition(survival::Inventory& joe,
                     const std::string& ammunitionItemId,
                     companion::CompanionCore& recipient,
                     const std::string& eventId, double atSeconds) {
  using companion::Experience;
  if (!joe.IsCategory(ammunitionItemId, survival::ItemCategory::Ammunition) ||
      joe.Quantity(ammunitionItemId) < 1 ||
      !recipient.CanApplyExperience(Experience::JoeSharedAmmo, eventId,
                                    atSeconds)) {
    return false;
  }

  // All rejection paths are checked before either state is changed. The game
  // thread must own both objects for the duration of this synchronous call.
  if (!recipient.ApplyExperience(Experience::JoeSharedAmmo, eventId,
                                  atSeconds)) {
    throw std::logic_error("Validated companion share unexpectedly failed");
  }
  if (!joe.Remove(ammunitionItemId, 1, survival::RemovalReason::Transfer)) {
    throw std::logic_error("Validated ammunition removal unexpectedly failed");
  }
  return true;
}

} // namespace thug::integration
