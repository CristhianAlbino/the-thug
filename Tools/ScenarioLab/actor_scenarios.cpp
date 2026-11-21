#include "../WorldCore/WorldCore.h"
#include "../SaveCore/SaveCore.h"
#include "../IntegrationCore/IntegrationCore.h"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace thug;

namespace {

int checks = 0;
void Check(bool value, const char* message) {
  ++checks;
  if (!value) throw std::runtime_error(message);
}

save::WorldObjectDefinition Object(const char* id, const char* type,
                                    const char* state, double x) {
  return {id, type, {state, {x, 0, 0}, {}, true}};
}

companion::Traits TestTraits() { return {50, 50, 50, 45, 50, 50, 0, false}; }

bool HasGoal(const std::vector<companion::Option>& options, companion::Goal goal) {
  for (const auto& option : options) if (option.goal == goal) return true;
  return false;
}

struct Scene {
  explicit Scene(std::uint64_t seed)
      : harley(companion::Character::Harley, companion::Difficulty::Hard,
               TestTraits(), seed + 1),
        barth(companion::Character::Bartholomeu, companion::Difficulty::Hard,
              TestTraits(), seed + 2),
        jack(companion::Character::Jack, companion::Difficulty::Hard,
             TestTraits(), seed + 3),
        batman(hunter::Difficulty::Hard, seed + 4), hearing({}, seed + 5) {}

  survival::Inventory joe{4, {{"magazine", survival::ItemCategory::Ammunition,
                                  1, 4}}};
  companion::CompanionCore harley;
  companion::CompanionCore barth;
  companion::CompanionCore jack;
  hunter::HunterCore batman;
  rules::RunRules rules{rules::Difficulty::Hard};
  survival::SurvivalCore survival;
  perception::PerceptionCore hearing;
  perception::EvidenceMemory evidence;
  save::WorldState state{"hospital", {
      Object("C_SECURITY", "circuit", "on", 0),
      Object("C_SERVICE", "circuit", "off", 1),
      Object("D_F2_SERVER", "door", "closed", 2),
      Object("D_G_CONTROL", "door", "closed", 3),
      Object("D_F3_EXAMS", "door", "locked", 5),
      Object("CAM_LOBBY", "camera", "online", 6),
      Object("CAM_GARAGE", "camera", "looped", 7),
      Object("H_F3_ISOLATION", "hide", "empty", 8)}};
  world::WorldCore world{state,
      {{"D_F2_SERVER", "F2_CENTRAL", "F2_SERVER", "C_SECURITY",
        world::DoorPower::RequiresPower},
       {"D_G_CONTROL", "G_FLOOR", "G_CONTROL", "C_SERVICE",
        world::DoorPower::ReleasesWithoutPower},
       {"D_F3_EXAMS", "F3_EXAMS", "F3_WINDOW", "",
        world::DoorPower::Manual}},
      {{"CAM_LOBBY", "F2_CAMERAS", "F1_LOBBY", "C_SECURITY"},
       {"CAM_GARAGE", "F2_CAMERAS", "G_FLOOR", "C_SERVICE"}},
      {{"H_F3_ISOLATION", "F3_ISOLATION"}}};

  save::Campaign Members() {
    return {{joe, harley, barth, jack}, batman, rules, survival,
            hearing, evidence, state};
  }
};

std::string Save(Scene& scene) {
  std::ostringstream stream;
  Check(save::SaveCampaign(stream, scene.Members()) == save::Result::Ok,
        "Full campaign checkpoint succeeds");
  return stream.str();
}

void AudibleGameplay(std::uint64_t seed) {
  Scene s(seed);
  hunter::HunterCore normal(hunter::Difficulty::Normal);
  const auto door = s.world.OpenDoor("D_F2_SERVER", 1);
  Check(door.has_value(), "Normal door opening produces a world sound");
  const perception::SoundEvent doorSound{
      door->sound, door->positionMeters, door->atSeconds, door->energy};
  auto heard = s.hearing.Hear(doorSound, {{7, 0, 0}, 1, 0});
  Check(heard && normal.Observe(*heard) &&
        normal.CurrentState() == hunter::State::Investigating,
        "Ordinary nearby door can actually trigger Normal Hunter investigation");
  Check(!s.hearing.Hear(doorSound, {{7, 0, 0}, 0, 0}),
        "Fully occluded ordinary door still provides no information");
  heard = s.hearing.Hear({perception::SoundKind::WalkFootstep, {5, 0, 0}, 2, 1},
                        {{0, 0, 0}, 1, 0});
  Check(heard && s.batman.Observe(*heard) &&
        s.batman.CurrentState() == hunter::State::Investigating,
        "Walking at real nonzero distance can trigger Hard Hunter");

  hunter::HunterCore calm(hunter::Difficulty::Normal), injured(hunter::Difficulty::Normal);
  const auto breath = perception::MakeBreathingEvent(
      {2, 0, 0}, 3, s.survival.CurrentEffects().breathingLoudnessMultiplier);
  const auto quiet = s.hearing.Hear(breath, {{0, 0, 0}, 1, 0});
  Check(quiet && calm.Observe(*quiet) && calm.CurrentState() == hunter::State::Patrol,
        "Calm breathing remains below Normal reaction threshold");
  Check(s.survival.ApplyInjury(survival::Injury::Severe), "Joe suffers an actual severe injury");
  heard = s.hearing.Hear(perception::MakeBreathingEvent(
      {2, 0, 0}, 4, s.survival.CurrentEffects().breathingLoudnessMultiplier),
      {{0, 0, 0}, 1, 0});
  Check(heard && heard->strength > quiet->strength && injured.Observe(*heard) &&
        injured.CurrentState() == hunter::State::Investigating,
        "Survival injury amplifies breathing enough to change Hunter behavior");
  Check(!s.hearing.Hear(perception::MakeBreathingEvent({100, 0, 0}, 5, 3),
                       {{0, 0, 0}, 1, 0}), "Even labored breathing fades at distance");
}

void Stage(Scene& s) {
  Check(s.joe.Add("magazine", 2), "Joe has transferable ammunition");
  const auto nearbyRoute = companion::Situation{
      false, true, false, true, false, std::string("tunnel")};
  Check(!HasGoal(s.barth.AvailableOptions(nearbyRoute),
                 companion::Goal::EscapeAlone),
        "Bartholomeu cannot use a route he has not learned");
  Check(s.world.ReadCamera("CAM_LOBBY") &&
            s.world.ReadCamera("CAM_LOBBY")->live,
        "Live lobby view exists before blackout");
  Check(s.batman.CurrentIntent().state == hunter::State::Patrol &&
            !s.batman.CurrentIntent().destination,
        "Batman has no authoritative Joe position at start");
  Check(!s.evidence.Inspect("C_SECURITY", "on", {0, 0, 0}, 5),
        "First witnessed circuit state establishes a baseline");

  const auto relay = s.world.SetCircuit("C_SECURITY", false, 10);
  Check(relay.has_value() && !s.world.ReadCamera("CAM_LOBBY") &&
            !s.world.OpenDoor("D_F2_SERVER", 11),
        "Outage changes information and access together");
  const auto silent = s.hearing.Hear({relay->sound, relay->positionMeters,
                                     relay->atSeconds, relay->energy},
                                    {{12, 0, 0}, 0.1, 1.0});
  Check(!silent && s.batman.CurrentState() == hunter::State::Patrol,
        "Masked relay does not inform Batman");
  const auto garageDoor = s.world.OpenDoor("D_G_CONTROL", 20);
  Check(garageDoor && s.world.CanTraverse("D_G_CONTROL"),
        "Unpowered service door remains usable");
  Check(s.world.EnterHide("H_F3_ISOLATION", 30) &&
            s.batman.HideSpotSuspicion("H_F3_ISOLATION") == 0,
        "Unseen entry stays private even when stored in world state");

  const auto crash = s.world.OpenDoor("D_F3_EXAMS", 35, true);
  Check(crash && crash->sound == perception::SoundKind::BrokenObject,
        "Forced access creates a loud physical signal");
  const auto heard = s.hearing.Hear({crash->sound, crash->positionMeters,
                                    crash->atSeconds, crash->energy},
                                   {{0, 0, 0}, 1.0, 0});
  Check(heard && heard->estimate && s.batman.Observe(*heard) &&
            s.batman.CurrentState() == hunter::State::Investigating,
        "Audible break reaches Hunter through uncertain perception");
  const auto destination = s.batman.CurrentIntent().destination;
  Check(destination && destination->radiusMeters >= 4 &&
            (destination->center.x != crash->positionMeters.x ||
             destination->center.y != crash->positionMeters.y) &&
            s.batman.HideSpotSuspicion("H_F3_ISOLATION") == 0,
        "Sound estimate is not exact and does not reveal hiding spot");

  // Only after an authored line-of-sight observation can Batman learn that
  // Joe reused this hide spot. The world state alone never emits this clue.
  Check(s.world.LeaveHide("H_F3_ISOLATION", 39) &&
            s.world.EnterHide("H_F3_ISOLATION", 40) &&
            s.batman.Observe({hunter::StimulusKind::ConfirmedHideSpot,
                              hunter::Subject::Player, 40, std::nullopt,
                              1, "H_F3_ISOLATION"}) &&
            s.batman.HideSpotSuspicion("H_F3_ISOLATION") > 0,
        "Witnessed re-entry raises suspicion");
  Check(s.barth.LearnRoute("tunnel", 45) &&
            HasGoal(s.barth.AvailableOptions(nearbyRoute),
                    companion::Goal::EscapeAlone) &&
            HasGoal(s.barth.AvailableOptions(nearbyRoute),
                    companion::Goal::ShareRoute),
        "Only explicit discovery unlocks companion route choices");
  Check(integration::ShareAmmunition(s.joe, "magazine", s.harley,
                                     "share_at_clinic", 46) &&
            s.joe.Quantity("magazine") == 1 &&
            s.harley.CurrentTraits().ammunition == 1,
        "Resource and trust consequences commit together");
  Check(s.world.SetCircuit("C_SERVICE", true, 47).has_value() &&
            s.world.ReadCamera("CAM_GARAGE") &&
            !s.world.ReadCamera("CAM_GARAGE")->live,
        "New power does not make a looped feed live");
  const auto inspected = s.evidence.Inspect("C_SECURITY", "off", {0, 0, 0}, 48);
  Check(inspected && s.batman.Observe(*inspected),
        "Batman learns blackout only after inspecting the changed object");
  s.batman.AdvanceTo(50);
  s.rules.AdvanceTo(50);
  Check(s.state.AdvanceTo(50), "World clock reaches checkpoint");
}

void Continue(Scene& s) {
  Check(s.barth.ApplyExperience(companion::Experience::JoeAbandonedCompanion,
                                "left_at_pumps", 60),
        "Abandonment has persistent consequences");
  companion::Influence strain;
  strain.loyalty = -10; // Provisional authored event; not a GDD numeric canon.
  Check(s.barth.ApplyInfluence(strain, "lost_contact", 61),
        "Narrative pressure is uniquely keyed");
  const auto situation = companion::Situation{
      true, true, false, true, false, std::string("tunnel")};
  Check(HasGoal(s.barth.AvailableOptions(situation),
                companion::Goal::UseJoeAsDistraction),
        "Low trust and observed Hunter permit betrayal as an option");
  Check(s.rules.Apply(rules::Event::TunnelFound, 65),
        "Joe learns tunnel only after an explicit verified disclosure");
  Check(s.state.AdvanceTo(65), "World clock includes late route and companion events");
}

} // namespace

int main(int argc, char** argv) {
  try {
    const std::uint64_t seed = argc > 1 ? std::stoull(argv[1]) : 42;
    AudibleGameplay(seed);
    Scene initial(seed);
    Stage(initial);
    const auto bytes = Save(initial);
    Scene loaded(seed);
    std::istringstream input(bytes);
    Check(save::LoadCampaign(input, loaded.Members()) == save::Result::Ok &&
              Save(loaded) == bytes,
          "Ten-system checkpoint restores world, knowledge and RNG");
    Check(loaded.world.Occupied("H_F3_ISOLATION") &&
              !loaded.world.ReadCamera("CAM_LOBBY") &&
              loaded.batman.HideSpotSuspicion("H_F3_ISOLATION") > 0 &&
              loaded.barth.KnownRoutes().count("tunnel") == 1,
          "Rehydrated objects and actors retain distinct facts");
    for (int i = 0; i < 12; ++i) {
      Check(initial.batman.ChoosePatrolRoute({"east", "west", "service"}) ==
                loaded.batman.ChoosePatrolRoute({"east", "west", "service"}),
            "Hunter random choices replay after checkpoint");
      const companion::Situation situation{
          true, true, false, true, false, std::string("tunnel")};
      Check(initial.barth.Choose(situation).goal ==
                loaded.barth.Choose(situation).goal,
            "Companion choices replay after checkpoint");
    }
    Continue(initial);
    Continue(loaded);
    Check(Save(initial) == Save(loaded),
          "Consequences after restored checkpoint stay identical");
    std::cout << "{\"scenario\":\"blackout_hunt_and_companion\","
              << "\"seed\":" << seed << ",\"checks\":" << checks
              << ",\"batman\":\"uncertain_sound_then_observed_hide\","
              << "\"camera\":\"offline\",\"companion\":\"knows_tunnel\","
              << "\"save_replay\":true}\n";
  } catch (const std::exception& error) {
    std::cerr << "Actor scenario failed after " << checks << " checks: "
              << error.what() << '\n';
    return 1;
  }
}
