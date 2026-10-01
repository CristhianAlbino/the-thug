#include "SaveCore.h"
#include "../IntegrationCore/IntegrationCore.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace thug;

namespace {

int checks = 0;

void Check(bool condition, const char* reason) {
  ++checks;
  if (!condition) throw std::runtime_error(reason);
}

companion::Traits Neutral() { return {50, 50, 50, 50, 50, 50, 0, false}; }

struct Fixture {
  survival::Inventory joe{6,
      {{"magazine", survival::ItemCategory::Ammunition, 1, 5},
       {"bandage", survival::ItemCategory::Medical, 1, 3}}};
  companion::CompanionCore harley{companion::Character::Harley,
                                   companion::Difficulty::Hard, Neutral(), 11};
  companion::CompanionCore barth{companion::Character::Bartholomeu,
                                 companion::Difficulty::Hard, Neutral(), 22};
  companion::CompanionCore jack{companion::Character::Jack,
                                companion::Difficulty::Hard, Neutral(), 33};
  hunter::HunterCore hunter{hunter::Difficulty::Hard, 44};
  rules::RunRules routes{rules::Difficulty::Hard};
  survival::SurvivalCore survival;
  perception::PerceptionCore hearing{{}, 55};
  perception::EvidenceMemory evidence;
  save::WorldState world{"hospital_test",
      {{"door-a", "door", {"closed", {1, 2, 0}, {0, 0, 90}, true}},
       {"key-a", "pickup", {"available", {4, 2, 0}, {}, true}},
       {"joe", "character", {"idle", {0, 0, 0}, {}, true}}}};

  save::Campaign Members() {
    return {{joe, harley, barth, jack}, hunter, routes, survival,
            hearing, evidence, world};
  }
};

std::string Encode(save::Campaign campaign) {
  std::ostringstream out;
  Check(save::SaveCampaign(out, campaign) == save::Result::Ok,
        "Campaign save succeeds");
  return out.str();
}

save::Result Decode(const std::string& data, save::Campaign campaign) {
  std::istringstream in(data);
  return save::LoadCampaign(in, campaign);
}

void PlayScenario(Fixture& f) {
  Check(f.joe.Add("magazine", 3) && f.joe.Add("bandage", 1),
        "Starting supplies available");
  Check(integration::ShareAmmunition(f.joe, "magazine", f.harley,
                                     "share-1", 5), "First share completed");
  Check(f.barth.LearnRoute("tunnel", 6), "Bartholomeu knows tunnel");
  Check(f.jack.ApplyExperience(
            companion::Experience::JoeAbandonedCompanion, "abandon-1", 7),
        "Jack remembers abandonment");

  const auto sound = f.hearing.Hear(
      {perception::SoundKind::Gunshot, {12, 2, 0}, 8},
      {{0, 0, 0}, 0.8, 0.1});
  Check(sound.has_value() && f.hunter.Observe(*sound),
        "Uncertain heard location reaches Hunter");
  Check(f.hunter.Observe({hunter::StimulusKind::ObservedRoute,
                          hunter::Subject::Player, 9, std::nullopt, 1,
                          "garage"}), "Hunter observes garage route");
  Check(f.hunter.Observe({hunter::StimulusKind::ConfirmedHideSpot,
                          hunter::Subject::Player, 10, std::nullopt, 1,
                          "cabinet"}), "Hunter confirms hiding place");
  Check(!f.evidence.Inspect("camera", "working", {0, 0, 0}, 10),
        "First inspection is baseline");
  const auto change = f.evidence.Inspect("camera", "broken",
                                         {0, 0, 0}, 11);
  Check(change.has_value() && f.hunter.Observe(*change),
        "Observed change becomes Hunter evidence");

  Check(f.routes.Apply(rules::Event::RadioFound, 12) &&
            f.routes.Apply(rules::Event::BatteriesInstalled, 13) &&
            f.routes.Apply(rules::Event::RadioContactEstablished, 14) &&
            f.routes.Apply(rules::Event::HelicopterConfirmed, 15),
        "Roof sequence has a live deadline");
  Check(f.routes.Apply(rules::Event::SafeOpened, 16) &&
            f.routes.Apply(rules::Event::GarageKeyTaken, 17) &&
            f.routes.Apply(rules::Event::TunnelFound, 18) &&
            f.routes.Apply(rules::Event::TunnelIntercepted, 19),
        "Other escape routes hold intermediate states");
  f.routes.AdvanceTo(100);

  Check(f.survival.ApplyInjury(survival::Injury::Severe),
        "Joe takes severe injury");
  const auto advanced = f.survival.Advance(3, survival::Activity::Sprint);
  Check(advanced.sprintSeconds > 0 && f.survival.IsBleeding() &&
            f.survival.BloodLoss() > 0 && f.survival.IsExhausted(),
        "Bleeding and exhaustion evolve before save");
  Check(f.world.SetObject("door-a",
          {"open", {1, 2, 0}, {0, 0, 0}, true}, 20) &&
        f.world.SetObject("key-a",
          {"collected", {4, 2, 0}, {}, false}, 21) &&
        f.world.SetObject("joe",
          {"crouching", {3, 2, 0}, {0, 0, 45}, true}, 22),
        "World remembers door, collected pickup and character transform");
  Check(f.world.AdvanceTo(100), "World clock covers all rule events");
}

void FullRoundTripAndContinuation() {
  Fixture original;
  PlayScenario(original);
  const auto saveFile = Encode(original.Members());
  Check(saveFile.rfind("THE_THUG_CAMPAIGN 1\n", 0) == 0,
        "Campaign has separate version header");

  Fixture resumed;
  Check(resumed.joe.Add("magazine", 5), "Resumed session initially differs");
  resumed.routes.AdvanceTo(200);
  resumed.hunter.AdvanceTo(200);
  resumed.evidence.Inspect("door", "locked", {0, 0, 0}, 200);
  Check(resumed.world.SetObject("door-a",
            {"locked", {1, 2, 0}, {0, 0, 90}, true}, 200),
        "World initially differs");
  Check(Decode(saveFile, resumed.Members()) == save::Result::Ok,
        "Earlier checkpoint loads over later session");
  Check(Encode(resumed.Members()) == saveFile,
        "All ten state snapshots round trip byte for byte");

  Check(resumed.joe.Quantity("magazine") == 2 &&
            resumed.harley.CurrentTraits().ammunition == 1 &&
            !integration::ShareAmmunition(resumed.joe, "magazine",
                                           resumed.harley, "share-1", 101),
        "Reloading cannot repeat the handoff");
  Check(resumed.joe.Quantity("magazine") == 2,
        "Rejected replay did not spend ammo");
  Check(resumed.hunter.RouteEvidence("garage") > 0 &&
            resumed.hunter.HideSpotSuspicion("cabinet") > 0 &&
            resumed.hunter.LastHeardType() == "gunshot" &&
            resumed.hunter.CurrentState() == hunter::State::Investigating,
        "Hunter memory, heard type and state restored");
  Check(resumed.routes.HelicopterDeadlineSeconds() == 315 &&
            resumed.routes.Status(rules::Route::Roof).ready &&
            resumed.routes.Status(rules::Route::Garage).discovered &&
            resumed.routes.Status(rules::Route::Tunnel).failed,
        "Roof deadline and partial route failures restored");
  Check(resumed.survival.CurrentInjury() == survival::Injury::Severe &&
            resumed.survival.IsBleeding() && resumed.survival.IsExhausted() &&
            resumed.survival.BloodLoss() == original.survival.BloodLoss(),
        "Survival condition restored");
  Check(resumed.world.Find("door-a") &&
            resumed.world.Find("door-a")->stateId == "open" &&
            resumed.world.Find("key-a") &&
            !resumed.world.Find("key-a")->present &&
            resumed.world.Find("joe") &&
            resumed.world.Find("joe")->positionMeters.x == 3,
        "Physical actor states and Joe position restored");
  Check(!resumed.evidence.Inspect("camera", "broken", {0, 0, 0}, 12),
        "Same inspected state is not rediscovered after load");
  Check(resumed.evidence.Inspect("camera", "offline", {0, 0, 0}, 13)
            .has_value(), "New visible change remains detectable");

  const auto heardAgain = perception::SoundEvent{
      perception::SoundKind::Door, {3, 4, 0}, 101};
  const auto listener = perception::ListenerContext{{0, 0, 0}, 1, 0};
  const auto nextOriginal = original.hearing.Hear(heardAgain, listener);
  const auto nextResumed = resumed.hearing.Hear(heardAgain, listener);
  Check(nextOriginal && nextResumed &&
            nextOriginal->estimate->center.x == nextResumed->estimate->center.x &&
            nextOriginal->estimate->center.y == nextResumed->estimate->center.y,
        "Perception RNG continues from saved state");
  for (int i = 0; i < 20; ++i) {
    Check(original.hunter.ChoosePatrolRoute({"garage", "roof", "tunnel"}) ==
              resumed.hunter.ChoosePatrolRoute({"garage", "roof", "tunnel"}),
          "Hunter route RNG continues from checkpoint");
  }
  Check(integration::ShareAmmunition(resumed.joe, "magazine",
                                     resumed.harley, "share-2", 101),
        "New handoff after load still works");
  resumed.routes.AdvanceTo(316);
  Check(resumed.routes.Status(rules::Route::Roof).failed &&
            !resumed.routes.Status(rules::Route::Garage).failed,
        "Restored deadline later closes only the roof route");
  Check(resumed.survival.Stabilize() && !resumed.survival.IsBleeding(),
        "Restored injury can still be treated");
}

void FailureLeavesAllTenUntouched() {
  Fixture fixture;
  PlayScenario(fixture);
  const auto clean = Encode(fixture.Members());
  const auto Reject = [&](const std::string& bytes, save::Result expected) {
    Check(Decode(bytes, fixture.Members()) == expected,
          "Rejected file has expected error");
    Check(Encode(fixture.Members()) == clean,
          "Rejected file did not mutate any live state");
  };
  Reject(clean.substr(0, clean.size() - 15), save::Result::Malformed);
  auto corrupt = clean;
  corrupt[corrupt.find("HUNTER_TUNING ") + 14] = '9';
  Reject(corrupt, save::Result::Corrupt);
  auto future = clean;
  future.replace(0, std::string("THE_THUG_CAMPAIGN 1").size(),
                 "THE_THUG_CAMPAIGN 2");
  Reject(future, save::Result::UnsupportedVersion);
  Reject(clean + "EXTRA", save::Result::Malformed);

  perception::Tuning changed;
  changed.minimumAudibleStrength = 0.04;
  perception::PerceptionCore incompatibleHearing(changed, 55);
  save::Campaign target{{fixture.joe, fixture.harley, fixture.barth,
                         fixture.jack}, fixture.hunter, fixture.routes,
                        fixture.survival, incompatibleHearing,
                        fixture.evidence, fixture.world};
  const auto before = Encode(target);
  Check(Decode(clean, target) == save::Result::IncompatibleRoster,
        "Late perception tuning mismatch rejects entire load");
  Check(Encode(target) == before,
        "Late validation failure did not mutate party, Hunter or routes");

  save::WorldState incompatibleWorld("other_level", {});
  Check(incompatibleWorld.AdvanceTo(100), "Other world clock is current");
  save::Campaign wrongWorld{{fixture.joe, fixture.harley, fixture.barth,
                             fixture.jack}, fixture.hunter, fixture.routes,
                            fixture.survival, fixture.hearing,
                            fixture.evidence, incompatibleWorld};
  const auto beforeWorld = Encode(wrongWorld);
  Check(Decode(clean, wrongWorld) == save::Result::IncompatibleRoster,
        "Different level catalog rejects full campaign load");
  Check(Encode(wrongWorld) == beforeWorld,
        "Final world validation failure changes no live state");

  std::ostringstream oldParty;
  Check(save::SaveParty(oldParty, fixture.Members().party) == save::Result::Ok,
        "Legacy party save API remains usable");
  Reject(oldParty.str(), save::Result::Malformed);
}

void WorldGuardsAndClock() {
  Fixture fixture;
  Check(!fixture.world.SetObject("missing",
            {"open", {}, {}, true}, 1),
        "Unknown actor cannot enter checkpoint");
  Check(!fixture.world.SetObject("door-a",
            {"", {}, {}, true}, 1),
        "World state must have an authored ID");
  Check(!fixture.world.SetObject("door-a",
            {"open", {INFINITY, 0, 0}, {}, true}, 1),
        "Nonfinite transform is rejected");
  Check(!fixture.world.AdvanceTo(-1), "World clock cannot move backward");
  Check(fixture.world.Find("door-a") &&
            fixture.world.Find("door-a")->stateId == "closed",
        "Invalid world updates leave object unchanged");
  fixture.routes.AdvanceTo(10);
  std::ostringstream out;
  Check(save::SaveCampaign(out, fixture.Members()) ==
            save::Result::IncompatibleRoster,
        "Cannot save rules ahead of the world clock");
  Check(out.str().empty(), "Inconsistent checkpoint writes no bytes");
  Check(fixture.world.AdvanceTo(10) &&
            save::SaveCampaign(out, fixture.Members()) == save::Result::Ok,
        "World clock catches up and checkpoint becomes valid");
}

void FatalInjuryRequiresCampaignOutcome() {
  Fixture fixture;
  fixture.survival.ApplyInjury(survival::Injury::Severe);
  Check(fixture.survival.Advance(1000, survival::Activity::Rest).becameDead &&
        fixture.world.AdvanceTo(1000), "Fatal bleeding is a real survival outcome");
  std::ostringstream out;
  Check(save::SaveCampaign(out, fixture.Members()) ==
        save::Result::IncompatibleRoster && out.str().empty(),
        "Death cannot be checkpointed while campaign still says ongoing");
  Check(fixture.routes.Apply(rules::Event::JoeDied, 1000),
        "Coordinator delivers confirmed death before checkpoint");
  const auto bytes = Encode(fixture.Members());
  Fixture loaded;
  Check(Decode(bytes, loaded.Members()) == save::Result::Ok &&
        loaded.survival.CurrentEffects().dead &&
        loaded.routes.Result() == rules::Ending::Died,
        "Confirmed death restores survival and ending together");
}

} // namespace

int main() {
  try {
    FullRoundTripAndContinuation();
    FailureLeavesAllTenUntouched();
    WorldGuardsAndClock();
    FatalInjuryRequiresCampaignOutcome();
    std::cout << "CampaignSave: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "CampaignSave test failed after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
