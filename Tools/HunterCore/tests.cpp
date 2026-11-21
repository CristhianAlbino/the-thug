#include "HunterCore.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace thug::hunter;

namespace {

int assertions = 0;

void Check(bool value, const char* reason) {
  ++assertions;
  if (!value) {
    throw std::runtime_error(reason);
  }
}

Stimulus Located(StimulusKind kind, Subject subject, double time,
                 double radius, double strength = 1) {
  return {kind, subject, time, LocationEstimate{{7, 8, 0}, radius, time},
          strength, ""};
}

Stimulus Named(StimulusKind kind, double time, const std::string& id,
               Subject subject = Subject::Player) {
  return {kind, subject, time, std::nullopt, 1, id};
}

void PerceptionContract() {
  HunterCore core(Difficulty::Hard);
  Check(!core.Observe(Located(StimulusKind::Sound, Subject::Unknown, 0, 0)),
        "Sound cannot give exact coordinates");
  Check(!core.Observe(Located(StimulusKind::Sound, Subject::Unknown, 0, 3)),
        "Sound must have minimum uncertainty");
  Check(core.CurrentState() == State::Patrol, "Invalid input must not mutate");
  Check(core.Observe(Located(StimulusKind::Sound, Subject::Decoy, 1, 7)),
        "Decoy sound must be allowed");
  Check(core.CurrentState() == State::Investigating,
        "Decoy can mislead the Hunter");
  Check(core.CurrentIntent().destination->radiusMeters == 7,
        "Decision retains uncertainty");
  Check(core.HideSpotSuspicion("locker") == 0 &&
            core.RouteEvidence("west") == 0,
        "Hearing a sound must not reveal habits");
  Check(!core.Observe(Named(StimulusKind::ConfirmedHideSpot, 0, "locker")),
        "Out-of-order evidence must be rejected");
  Check(!core.Observe(Named(StimulusKind::ConfirmedHideSpot, 2, "locker",
                            Subject::Unknown)),
        "Unknown actor cannot prove a player hide spot");
  Check(core.HideSpotSuspicion("locker") == 0,
        "Rejected evidence must not mutate memory");
}

void PursuitAndLoss() {
  HunterCore core(Difficulty::Hard);
  Check(core.Observe(Located(StimulusKind::VisualContact, Subject::Player, 5, 0)),
        "Visual contact accepted");
  Check(core.CurrentState() == State::Pursuing, "Player seen: pursue");
  Check(core.Observe(Located(StimulusKind::Sound, Subject::Decoy, 6, 8)),
        "Sound is valid during pursuit");
  Check(core.CurrentIntent().destination->radiusMeters == 0,
        "Decoy cannot replace live visual target");
  Check(core.Observe(Located(StimulusKind::VisualContact, Subject::Companion, 7, 0)),
        "Companion sighting accepted");
  Check(core.CurrentState() == State::Pursuing,
        "Companion cannot override player pursuit");
  Check(core.Observe({StimulusKind::VisualLost, Subject::Player, 8,
                      std::nullopt, 1, ""}),
        "Loss of sight accepted");
  Check(core.CurrentState() == State::Searching, "Loss leads to search");
  Check(core.CurrentIntent().destination->observedAtSeconds == 5,
        "Search uses last actual player sighting");
  core.AdvanceTo(49);
  Check(core.CurrentState() == State::Patrol, "Search eventually stops");
  Check(!core.CurrentIntent().destination, "Old target is discarded");
}

void CompanionsAndTime() {
  HunterCore core(Difficulty::Hard);
  Check(core.Observe(Located(StimulusKind::VisualContact, Subject::Companion, 1, 0)),
        "Companion contact accepted");
  Check(core.CurrentState() == State::Investigating,
        "Companion is not treated as Joe");
  Check(core.RouteEvidence("west") == 0,
        "Companion contact does not establish Joe's route");
  core.AdvanceTo(17);
  Check(core.CurrentState() == State::Searching,
        "Investigation changes to search");
  core.AdvanceTo(60);
  Check(core.CurrentState() == State::Patrol,
        "Large time step processes both deadlines");
  bool threw = false;
  try { core.AdvanceTo(59); } catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Clock may not go backwards");
}

void LearnedEvidenceAndDecay() {
  HunterCore core(Difficulty::Hard, 42);
  for (int i = 0; i < 4; ++i) {
    Check(core.Observe(Named(StimulusKind::ConfirmedHideSpot, 0, "locker")),
          "Confirmed use accepted");
    const double expected[] = {10, 30, 60, 90};
    Check(std::abs(core.HideSpotSuspicion("locker") - expected[i]) < 1e-9,
          "GDD hide spot suspicion progression");
  }
  Check(core.HideSpotSuspicion("other") == 0,
        "Unknown hide spot stays unknown");
  Check(core.Observe(Named(StimulusKind::ObservedRoute, 0, "west")),
        "Seen route accepted");
  Check(core.Observe(Named(StimulusKind::ObservedRoute, 0, "west")),
        "Repeated route accepted");
  Check(core.RouteEvidence("west") == 2,
        "Only direct evidence increases route memory");
  core.AdvanceTo(300);
  Check(std::abs(core.RouteEvidence("west") - 1) < 1e-9,
        "Route memory loses half after its half-life");
  Check(std::abs(core.HideSpotSuspicion("locker") - 45) < 1e-9,
        "Hide spot memory also decays");
}

void ChoicesCanBeWrong() {
  HunterCore core(Difficulty::Punitive, 123);
  for (int i = 0; i < 100; ++i) {
    Check(core.Observe(Named(StimulusKind::ObservedRoute, 0, "west")),
          "Witnessed route event accepted");
  }
  int west = 0;
  int east = 0;
  for (int i = 0; i < 2000; ++i) {
    const std::string chosen = core.ChoosePatrolRoute({"west", "east"});
    west += chosen == "west";
    east += chosen == "east";
  }
  Check(west > east, "Observed route becomes more likely");
  Check(east > 0, "The Hunter never has perfect certainty");
  bool threw = false;
  try { core.ChoosePatrolRoute({"west", "west"}); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Invalid route list rejected");
}

void DifficultyIsBehavioral() {
  HunterCore easy(Difficulty::Easy), punitive(Difficulty::Punitive);
  auto faint = Located(StimulusKind::Sound, Subject::Unknown, 0, 10, 0.2);
  Check(easy.Observe(faint) && punitive.Observe(faint),
        "Both cores receive the same sound");
  Check(easy.CurrentState() == State::Patrol,
        "Easy ignores faint sound");
  Check(punitive.CurrentState() == State::Investigating,
        "Punitive investigates faint sound");
  Check(easy.Observe(Named(StimulusKind::ObservedRoute, 0, "west")) &&
            punitive.Observe(Named(StimulusKind::ObservedRoute, 0, "west")),
        "Both witness the same route");
  easy.AdvanceTo(90);
  punitive.AdvanceTo(90);
  Check(easy.RouteEvidence("west") < punitive.RouteEvidence("west"),
        "Higher difficulty remembers longer");
}

} // namespace

int main() {
  try {
    PerceptionContract();
    PursuitAndLoss();
    CompanionsAndTime();
    LearnedEvidenceAndDecay();
    ChoicesCanBeWrong();
    DifficultyIsBehavioral();
    std::cout << "HunterCore: " << assertions << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "HunterCore test failed after " << assertions
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
