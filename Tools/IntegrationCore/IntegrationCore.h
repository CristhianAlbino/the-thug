#pragma once

#include "../CompanionCore/CompanionCore.h"
#include "../SurvivalCore/Inventory.h"

#include <string>

namespace thug::integration {

// One item unit in Joe's catalog represents one transferable unit of ammo.
// Call only after the world has validated reachability and the handoff action.
// The same event ID must be persisted together with both states in a save.
bool ShareAmmunition(survival::Inventory& joe,
                     const std::string& ammunitionItemId,
                     companion::CompanionCore& recipient,
                     const std::string& eventId, double atSeconds);

} // namespace thug::integration
