#pragma once

#include "../CompanionCore/CompanionCore.h"
#include "../SurvivalCore/Inventory.h"

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

} // namespace thug::save
