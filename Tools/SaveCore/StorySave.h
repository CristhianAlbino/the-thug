#pragma once

#include "NarrativeState.h"
#include "SaveCore.h"

namespace thug::save {

struct StoryCampaign {
  Campaign core;
  NarrativeState& narrative;
};

// THE_THUG_STORY_CAMPAIGN 1 contains the complete v1 core and a narrative
// snapshot. Loading stages both; old core-only saves cannot become story saves.
Result SaveStoryCampaign(std::ostream& output, const StoryCampaign& campaign);
Result LoadStoryCampaign(std::istream& input, StoryCampaign campaign);

} // namespace thug::save
