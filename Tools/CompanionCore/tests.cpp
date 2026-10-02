#include "CompanionCore.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace thug::companion;

namespace {

int checks = 0;

void Check(bool value, const char* reason) {
  ++checks;
  if (!value) throw std::runtime_error(reason);
}

bool Has(const std::vector<Option>& options, Goal goal) {
  return std::any_of(options.begin(), options.end(),
                     [goal](const Option& option) { return option.goal == goal; });
}

Traits Neutral() { return {50, 50, 50, 50, 50, 50, 0, false}; }

void GddConsequencesAndIdempotence() {
  CompanionCore barth(Character::Bartholomeu, Difficulty::Hard, Neutral());
  Check(barth.ApplyExperience(Experience::JoeSharedAmmo, "shared-ammo-1", 1),
        "Ammo sharing accepted");
  Check(barth.CurrentTraits().trust == 70 &&
            barth.CurrentTraits().loyalty == 60 &&
            barth.CurrentTraits().ammunition == 1,
        "Exact GDD sharing deltas applied");
  Check(!barth.ApplyExperience(Experience::JoeSharedAmmo, "shared-ammo-1", 2),
        "Same event cannot grant ammo twice");
  Check(barth.CurrentTraits().ammunition == 1, "Duplicate did not mutate");
  Check(barth.ApplyExperience(Experience::JoeAbandonedCompanion,
                              "abandoned-1", 3),
        "Abandonment accepted");
  Check(barth.CurrentTraits().trust == 30 &&
            barth.CurrentTraits().fear == 70 &&
            barth.CurrentTraits().selfPreservation == 80,
        "Exact GDD abandonment deltas applied");
  Check(barth.CurrentTraits().loyalty == 60,
        "Unspecified loyalty is unchanged");
}

void KnowledgeIsIndividual() {
  CompanionCore harley(Character::Harley, Difficulty::Hard, Neutral());
  CompanionCore jack(Character::Jack, Difficulty::Hard, Neutral());
  Situation situation;
  situation.joePerceivedNearby = true;
  situation.accessibleEscapeRoute = "garage";
  Check(!Has(harley.AvailableOptions(situation), Goal::ShareRoute),
        "Cannot share unknown route");
  Check(!Has(harley.AvailableOptions(situation), Goal::EscapeAlone),
        "Cannot use unknown exit");
  Check(harley.LearnRoute("garage", 1), "Harley learns observed route");
  Check(!harley.LearnRoute("garage", 2), "Duplicate discovery ignored");
  Check(Has(harley.AvailableOptions(situation), Goal::ShareRoute),
        "Harley can share known route");
  Check(Has(harley.AvailableOptions(situation), Goal::EscapeAlone),
        "Harley can consider reachable known exit");
  Check(!Has(jack.AvailableOptions(situation), Goal::ShareRoute),
        "Jack does not magically inherit Harley's knowledge");
  Check(!Has(jack.AvailableOptions(situation), Goal::EscapeAlone),
        "Jack cannot use Harley's route");
}

void DifficultyAndSituationGuardrails() {
  Traits distressed{10, 95, 90, 15, 95, 20, 0, false};
  CompanionCore easy(Character::Bartholomeu, Difficulty::Easy, distressed);
  CompanionCore hard(Character::Bartholomeu, Difficulty::Hard, distressed);
  Check(easy.LearnRoute("tunnel", 1) && hard.LearnRoute("tunnel", 1),
        "Both learned same route");
  Situation danger;
  danger.hunterPerceivedNearby = true;
  danger.joePerceivedNearby = true;
  danger.accessibleEscapeRoute = "tunnel";
  Check(!Has(easy.AvailableOptions(danger), Goal::EscapeAlone),
        "Easy companions do not abandon the player");
  Check(!Has(easy.AvailableOptions(danger), Goal::UseJoeAsDistraction),
        "Easy companions do not betray");
  Check(Has(hard.AvailableOptions(danger), Goal::EscapeAlone),
        "Hard may consider solo escape");
  Check(Has(hard.AvailableOptions(danger), Goal::UseJoeAsDistraction),
        "Hard may consider betrayal under GDD conditions");
  danger.joePerceivedNearby = false;
  Check(!Has(hard.AvailableOptions(danger), Goal::UseJoeAsDistraction),
        "Cannot use an unperceived Joe as distraction");
  Check(!Has(hard.AvailableOptions(danger), Goal::ShareRoute),
        "Cannot speak to an unperceived Joe");
}

void InvalidActionsDoNotMutate() {
  CompanionCore core(Character::Jack, Difficulty::Normal, Neutral());
  Check(!core.ApplyInfluence({0, 0, 0, 0, 0, 0, -1}, "take-ammo", 1),
        "Cannot spend missing ammunition");
  Check(core.CurrentTraits().ammunition == 0, "Rejected spend unchanged");
  Check(!core.LearnRoute("", 1), "Empty route ID rejected");
  Check(!core.ApplyInfluence({}, "", 1), "Empty event ID rejected");
  Check(core.ApplyInfluence({100, -100, 0, 0, 0, 0, 0}, "reassured", 2),
        "Authored influence accepted");
  Check(core.CurrentTraits().trust == 100 && core.CurrentTraits().fear == 0,
        "Traits clamped to authored range");
  bool threw = false;
  try { core.SetInjured(true, 1); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Cannot receive an out-of-order injury");
  Check(!core.CurrentTraits().injured, "Invalid injury unchanged");
}

void ChoicesAreWeightedAndReproducible() {
  Traits bonded{90, 20, 20, 90, 30, 90, 1, false};
  CompanionCore first(Character::Harley, Difficulty::Hard, bonded, 99);
  CompanionCore second(Character::Harley, Difficulty::Hard, bonded, 99);
  Situation situation;
  situation.joePerceivedNearby = true;
  situation.joeNeedsHelp = true;
  situation.canReachJoe = true;
  situation.resourcesKnownNearby = true;
  int helped = 0;
  int hid = 0;
  for (int i = 0; i < 500; ++i) {
    const auto a = first.Choose(situation);
    const auto b = second.Choose(situation);
    Check(a.goal == b.goal, "Same seed and observations replay identically");
    helped += a.goal == Goal::HelpJoe;
    hid += a.goal == Goal::StayHidden;
  }
  Check(helped > hid, "Strong bond favors helping Joe");
  Check(hid > 0, "Companion choice is not a guaranteed script");
  first.SetInjured(true, 1);
  Check(!Has(first.AvailableOptions(situation), Goal::HelpJoe),
        "Injured companion cannot offer unreachable assistance");
}

void ExecutableOptionsValidateBeforeRng() {
  CompanionCore core(Character::Jack, Difficulty::Hard, {40,35,30,35,80,25}, 17);
  const auto original=core.CaptureSnapshot().randomState;
  const auto rejected=[&](std::vector<Option> options){
    bool threw=false;
    try { core.ChooseAmong(options); } catch(const std::invalid_argument&) { threw=true; }
    Check(threw,"Invalid executable policy input is rejected");
    Check(core.CaptureSnapshot().randomState==original,"Rejected policy never consumes RNG");
  };
  rejected({});
  rejected({{Goal::StayHidden,0,std::nullopt}});
  rejected({{Goal::StayHidden,std::numeric_limits<double>::infinity(),std::nullopt}});
  rejected({{static_cast<Goal>(99),1,std::nullopt}});
  rejected({{Goal::ShareRoute,1,std::nullopt}});
  rejected({{Goal::StayHidden,1,std::string("tunnel")}});
  rejected({{Goal::EscapeAlone,1,std::string("unknown")}});
  Check(core.LearnRoute("tunnel",0),"Filtered routes remain individual knowledge");
  Check(core.ChooseAmong({{Goal::ShareRoute,1,std::string("tunnel")}}).routeId==std::string("tunnel"),"Executable route option preserves actual known route");
  CompanionCore easy(Character::Jack,Difficulty::Easy,{70,20,20,70,55,30});
  easy.LearnRoute("tunnel",0);bool blocked=false;
  try { easy.ChooseAmong({{Goal::EscapeAlone,1,std::string("tunnel")}}); } catch(const std::invalid_argument&) { blocked=true; }
  Check(blocked,"Filtering cannot enable forbidden Easy escape");
}

} // namespace

int main() {
  try {
    GddConsequencesAndIdempotence();
    KnowledgeIsIndividual();
    DifficultyAndSituationGuardrails();
    InvalidActionsDoNotMutate();
    ChoicesAreWeightedAndReproducible();
    ExecutableOptionsValidateBeforeRng();
    std::cout << "CompanionCore: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "CompanionCore test failed after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
