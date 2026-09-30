#pragma once

#include "../CompanionCore/CompanionCore.h"
#include "../HunterCore/HunterCore.h"
#include "../HunterCore/RunRules.h"
#include "../PerceptionCore/PerceptionCore.h"
#include "../SurvivalCore/Inventory.h"
#include "../SurvivalCore/SurvivalCore.h"
#include "WorldState.h"

#include <iosfwd>

namespace thug::save {

// A party snapshot is one logical save unit. The same gameplay thread owns all
// four objects while saving or loading. World actors must be handled separately.
struct Party {
  survival::Inventory& joe;
  companion::CompanionCore& harley;
  companion::CompanionCore& bartholomeu;
  companion::CompanionCore& jack;
};

enum class Result {
  Ok, IoError, TooLarge, Malformed, Corrupt, UnsupportedVersion,
  IncompatibleRoster
};

Result SaveParty(std::ostream& output, const Party& party);
// Never changes any of the four live objects on a rejected input.
Result LoadParty(std::istream& input, Party party);

struct Campaign {
  Party party;
  hunter::HunterCore& hunter;
  rules::RunRules& routes;
  survival::SurvivalCore& survival;
  perception::PerceptionCore& hearing;
  perception::EvidenceMemory& evidence;
  WorldState& world;
};

// One versioned checkpoint for all current engine-independent systems.
// The Unreal world state (actors, transforms, doors, etc.) is a later layer.
Result SaveCampaign(std::ostream& output, const Campaign& campaign);
Result LoadCampaign(std::istream& input, Campaign campaign);

} // namespace thug::save
