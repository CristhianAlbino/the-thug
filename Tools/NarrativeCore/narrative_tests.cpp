// Mirrors the cases of validate_variants.py so the C++ port used by the game
// cannot drift from the Python contract.
#include "NarrativeCore.h"

#include <iostream>
#include <stdexcept>

using namespace thug::narrative;

namespace {

int assertions = 0;

void Check(bool condition, const char* message) {
  ++assertions;
  if (!condition) throw std::runtime_error(message);
}

template <typename F>
void Throws(F&& f, const char* message) {
  ++assertions;
  try {
    f();
  } catch (const std::invalid_argument&) {
    return;
  }
  throw std::runtime_error(message);
}

Facts At(Facts base, const std::string& node) {
  base.joeNode = node;
  return base;
}

void EarlyScenes() {
  Facts opening;
  opening.difficulty = Difficulty::Hard;
  opening.joeNode = "F4_START";
  Check(Eligible("N01", opening), "N01 at start");
  Check(!Eligible("N03", opening), "N03 needs N02");
  Facts signal = At(opening, "F4_OBSERVATION");
  signal.seen = {"N01"};
  Check(Eligible("N02", signal), "N02 at the observation window");
  Facts split = opening;
  split.seen = {"N01", "N02", "N03"};
  Check(Eligible("N04", At(split, "F4_HALL_E")), "N04 after split");
  Check(Eligible("N06", At(split, "F3_CENTRAL")), "N06 clinic");
  Check(Eligible("N09", At(split, "F2_SECURITY")), "N09 monitors");
  Check(!Eligible("N10", At(split, "F2_SERVER")), "N10 needs N09 or DOC11");
  Facts doc11 = At(split, "F2_SERVER");
  doc11.facts = {"doc11_read"};
  Check(Eligible("N10", doc11), "N10 through DOC11");

  Facts harley = At(split, "F3_ICU");
  Check(!Eligible("N07", harley), "N07 needs Harley");
  harley.companions["harley"] = {"F1_TREATMENT", true};
  Check(!Eligible("N07", harley), "N07 needs Harley in the same node");
  harley.companions["harley"] = {"F3_ICU", false};
  Check(!Eligible("N07", harley), "N07 needs Harley active");
  harley.companions["harley"] = {"F3_ICU", true};
  Check(Eligible("N07", harley), "N07 with Harley present");

  Facts jack = At(split, "F3_EXAMS");
  jack.companions["jack"] = {"F3_EXAMS", true};
  Check(Eligible("N08", jack), "N08 with Jack");
  Check(!CanWitnessJ1(jack, true), "J1 needs N08 seen");
  Facts met = jack;
  met.seen.insert("N08");
  met.difficulty = Difficulty::Punitive;
  Check(CanWitnessJ1(met, true), "J1 in Punitive");
  Check(!CanWitnessJ1(met, false), "J1 needs sight and sound");
  Facts normal = met;
  normal.difficulty = Difficulty::Normal;
  Check(!CanWitnessJ1(normal, true), "J1 never outside Punitive");
  Facts away = met;
  away.companions["jack"] = {"F2_CAMERAS", true};
  Check(!CanWitnessJ1(away, true), "J1 needs Jack beside Joe");

  Facts hunt = At(split, "F3_CENTRAL");
  hunt.hunterHasEvidence = hunt.hunterCanReach = hunt.alternativeReachable =
      hunt.areaPreviouslyVisited = true;
  Check(Eligible("N11", hunt), "N11 with evidence");
  Facts h = hunt;
  h.hunterHasEvidence = false;
  Check(!Eligible("N11", h), "N11 needs Hunter evidence");
  h = hunt;
  h.hunterCanReach = false;
  Check(!Eligible("N11", h), "N11 needs a reachable Hunter");
  h = hunt;
  h.alternativeReachable = false;
  Check(!Eligible("N11", h), "N11 needs an alternative exit");
  h = hunt;
  h.areaPreviouslyVisited = false;
  Check(!Eligible("N11", h), "N11 needs a visited area");
  h = hunt;
  h.seen.insert("N11");
  Check(!Eligible("N11", h), "N11 plays once");
}

void LateScenes() {
  Facts split;
  split.difficulty = Difficulty::Hard;
  split.joeNode = "F1_PHARMACY";
  split.seen = {"N01", "N02", "N03"};
  Check(Eligible("N12", split), "N12 pharmacy");
  Check(Eligible("N14", At(split, "F2_ARCHIVE")), "N14 fragment before key");
  Check(!Eligible("N15", At(split, "B1_PUMP")), "N15 needs Bartholomeu");
  Facts barth = At(split, "B1_PUMP");
  barth.companions["bartholomeu"] = {"B1_PUMP", true};
  Check(Eligible("N15", barth), "N15 with Bartholomeu");

  Facts habit = split;
  habit.facts = {"hunter_observed_habit"};
  Check(!Eligible("N21", habit), "N21 needs circulation and reach");
  habit = At(habit, "F3_CENTRAL");
  habit.hunterCanReach = habit.alternativeReachable = true;
  Check(Eligible("N21", habit), "N21 in circulation");
  Check(!Eligible("N21", At(habit, "F1_PHARMACY")), "N21 not in a room");

  Check(!Eligible("N22", At(split, "B1_GENERATOR")), "N22 needs a circuit change");
  Facts circuit = At(split, "B1_GENERATOR");
  circuit.facts = {"circuit_changed"};
  Check(Eligible("N22", circuit), "N22 circuit");

  Facts harley = At(split, "F3_RECOVERY");
  harley.companions["harley"] = {"F3_RECOVERY", true};
  Check(!Eligible("N23", harley), "N23 needs pressure");
  harley.facts = {"harley_under_pressure"};
  Check(Eligible("N23", harley), "N23 under pressure");
  harley.facts = {"harley_injured"};
  harley.companions["harley"].active = false;
  Check(!Eligible("N23", harley), "N23 needs active Harley");

  Facts failure = split;
  failure.routeFailure = "roof";
  failure.facts = {"route_failure_observed", "route_failed:roof"};
  Check(!Eligible("N26", failure), "N26 needs an alternative");
  failure.alternativeReachable = true;
  Check(Eligible("N26", failure), "N26 roof");
  Check(!Eligible("N26", At(failure, "X_ROOF")), "N26 not at an exit");
  Facts again = failure;
  again.seen.insert("N26:roof");
  Check(!Eligible("N26", again), "N26 once per route");
  again.routeFailure = "garage";
  again.facts.insert("route_failed:garage");
  Check(Eligible("N26", again), "N26 for another route");

  Check(!Eligible("N27", At(split, "F3_ISOLATION")), "N27 isolation needs knowledge");
  Check(Eligible("N27", At(split, "F3_UTILITY")), "N27 utility");
  Facts known = At(split, "F3_ISOLATION");
  known.facts = {"isolation_known"};
  Check(Eligible("N27", known), "N27 with knowledge");

  const char* routes[][3] = {{"N28", "X_GARAGE", "garage_escaped"},
                             {"N29", "X_ROOF", "roof_escaped"},
                             {"N30", "X_TUNNEL", "tunnel_escaped"},
                             {"N31", "X_WINDOW", "window_escaped"}};
  for (auto& r : routes) {
    Facts exit = At(split, r[1]);
    Check(Eligible(r[0], exit), "visiting an exit is eligible");
    Check(!CanCloseRoute(r[0], exit), "visit alone does not win");
    exit.facts = {r[2]};
    Check(CanCloseRoute(r[0], exit), "RunRules fact closes the route");
    Facts far = At(split, "F4_START");
    far.facts = {r[2]};
    Check(!CanCloseRoute(r[0], far), "route closes only at its exit");
  }

  Facts dawn = split;
  dawn.facts = {"dawn_victory"};
  Check(Eligible("N32", dawn), "N32 dawn");
  dawn.difficulty = Difficulty::Punitive;
  Check(!Eligible("N32", dawn), "no dawn in Punitive");
  dawn.facts = {"hide_victory"};
  Check(Eligible("N32", dawn), "hiding in Punitive");
  Facts rare = split;
  rare.facts = {"rare_confrontation"};
  Check(!Eligible("N33", rare), "confrontation is Easy only");
  rare.difficulty = Difficulty::Easy;
  Check(Eligible("N33", rare), "Easy confrontation");
  rare.difficulty = Difficulty::Punitive;
  rare.facts = {"rare_destruction"};
  Check(Eligible("N33", rare), "Punitive destruction");

  Facts morgue = At(split, "B1_MORGUE");
  morgue.facts = {"joe_identified_thomas_body"};
  Check(!Eligible("N34", morgue), "body alone is not proof");
  morgue.facts.insert("joe_verified_batman_record");
  Check(Eligible("N34", morgue), "body and record");
  Facts direct = At(morgue, "F3_SURGERY");
  direct.facts = {"joe_directly_witnessed_thomas_death"};
  Check(Eligible("N34", direct), "direct witness");
  Facts normal = morgue;
  normal.difficulty = Difficulty::Normal;
  Check(!Eligible("N34", normal), "N34 never in Normal");
  direct.difficulty = Difficulty::Easy;
  Check(!Eligible("N34", direct), "N34 never in Easy");

  Facts jack = At(split, "F2_CAMERAS");
  jack.difficulty = Difficulty::Punitive;
  jack.companions["jack"] = {"F2_CAMERAS", true};
  jack.seen.insert("N15");
  jack.seen.insert("N25");
  jack.facts = {"jack_act3_encounter", "jack_heard_thomas_or_injury", "jack_perceived_companion_risk"};
  Check(CanWitnessLateClue("J2", jack, 3, true), "J2");
  Facts hard = jack;
  hard.difficulty = Difficulty::Hard;
  Check(!CanWitnessLateClue("J2", hard, 3, true), "J2 Punitive only");
  Check(CanWitnessLateClue("J3", jack, 4, true), "J3");
  Facts alone = jack;
  alone.companions.clear();
  Check(!CanWitnessLateClue("J3", alone, 4, true), "J3 needs Jack");
  Facts j4 = jack;
  j4.facts.insert("J3");
  j4.facts.insert("jack_final_encounter");
  Check(!CanWitnessLateClue("J4", j4, 5, true), "J4 needs two clues");
  j4.facts.insert("J2");
  Check(CanWitnessLateClue("J4", j4, 5, true), "J4 with J2 and J3");
  Check(!CanWitnessLateClue("J4", j4, 4, true), "J4 only in act 5");
  Throws([&] { CanWitnessLateClue("J1", jack, 3, true); }, "J1 is not a late clue");
}

std::map<std::string, CompanionEvidence> Unknown() {
  std::map<std::string, CompanionEvidence> e;
  for (const char* p : {"harley", "bartholomeu", "jack"}) e[p].lastContactNode = "F3_RECOVERY";
  return e;
}

void Endings() {
  auto unknown = Unknown();
  for (auto& kv : DeriveOutcomes(unknown, true)) Check(kv.second == Status::Unknown, "silence is unknown");
  Check(ResolveEpilogue(Difficulty::Normal, DeriveOutcomes(unknown, true))["harley"] == "E_HARLEY_UNKNOWN",
        "Harley unknown");
  Check(ResolveEpilogue(Difficulty::Hard, DeriveOutcomes(unknown, true))["thomas"] == "E_THOMAS_CODA_PLAYER_ONLY",
        "Hard coda");
  Check(ResolveEpilogue(Difficulty::Easy, DeriveOutcomes(unknown, true))["thomas"] == "E_THOMAS_UNCONFIRMED",
        "Easy unconfirmed");

  auto saved = unknown;
  saved["harley"] = CompanionEvidence{true, false, false, false, ""};
  saved["bartholomeu"] = CompanionEvidence{false, true, false, false, ""};
  saved["jack"] = CompanionEvidence{false, false, true, false, ""};
  auto outcomes = DeriveOutcomes(saved, true);
  Check(outcomes["harley"] == Status::Together && outcomes["bartholomeu"] == Status::EscapedAlone &&
            outcomes["jack"] == Status::Captured,
        "verified outcomes");
  Check(ResolveEpilogue(Difficulty::Hard, outcomes)["harley"] != ResolveEpilogue(Difficulty::Punitive, outcomes)["harley"],
        "Harley together differs in Punitive");
  auto dead = saved;
  dead["jack"] = CompanionEvidence{false, false, false, true, ""};
  Check(ResolveEpilogue(Difficulty::Hard, DeriveOutcomes(dead, true))["jack"] == "E_JACK_DEAD", "Jack dead");
  Throws([&] { DeriveOutcomes(saved, false); }, "together needs Joe out");
  auto both = saved;
  both["jack"] = CompanionEvidence{false, false, true, true, ""};
  Throws([&] { DeriveOutcomes(both, true); }, "contradictory evidence");
  Throws([&] { ResolveEpilogue(Difficulty::Normal, DeriveOutcomes(unknown, true), true); },
         "Thomas knowledge cannot leak into Normal");
  Throws([&] { ResolveEpilogue(Difficulty::Normal, outcomes, false, true); },
         "Jack's secret cannot leak into Normal");
}

void Variants() {
  const Difficulty all[] = {Difficulty::Easy, Difficulty::Normal, Difficulty::Hard, Difficulty::Punitive};
  for (Difficulty d : all) {
    const bool revealed = d == Difficulty::Hard || d == Difficulty::Punitive;
    Check(SceneAvailable("N34", d) == revealed, "N34 by difficulty");
    Check(SceneAvailable("J1", d, {}, true, 2) == (d == Difficulty::Punitive), "J1 by difficulty");
    Check(SceneAvailable("J4", d, {"J1", "J3"}, true, 5) == (d == Difficulty::Punitive), "J4 by difficulty");
    std::map<std::string, Status> unknown = {
        {"harley", Status::Unknown}, {"bartholomeu", Status::Unknown}, {"jack", Status::Unknown}};
    Check(ResolveEpilogue(d, unknown)["thomas"] ==
              (revealed ? "E_THOMAS_CODA_PLAYER_ONLY" : "E_THOMAS_UNCONFIRMED"),
          "Thomas epilogue by difficulty");
    if (revealed) {
      Check(ResolveEpilogue(d, unknown, true)["thomas"] == "E_THOMAS_JOE_KNOWS", "Joe knows");
    }
  }
  Check(!SceneAvailable("J1", Difficulty::Punitive, {}, true, 1), "J1 act 2");
  Check(!SceneAvailable("J2", Difficulty::Punitive, {}, true, 2), "J2 act 3");
  Check(SceneAvailable("J2", Difficulty::Punitive, {}, true, 3), "J2 in act 3");
  Check(!SceneAvailable("J3", Difficulty::Punitive, {"J1"}, true, 3), "J3 act 4");
  Check(SceneAvailable("J3", Difficulty::Punitive, {}, true, 4), "J3 in act 4");
  Check(!SceneAvailable("J4", Difficulty::Punitive, {"J1", "J2"}, true, 5), "J4 needs J3");
  Check(!SceneAvailable("J4", Difficulty::Punitive, {"J3"}, true, 5), "J4 needs two clues");
  Check(!SceneAvailable("J4", Difficulty::Punitive, {"J1", "J3"}, true, 4), "J4 act 5");
  Check(SceneAvailable("J4", Difficulty::Punitive, {"J2", "J3"}, true, 5), "J4");
  Check(!SceneAvailable("J4", Difficulty::Punitive, {"J2", "J3"}, false, 5), "J4 needs Jack");
  std::map<std::string, Status> base = {
      {"harley", Status::Together}, {"bartholomeu", Status::EscapedAlone}, {"jack", Status::Captured}};
  std::set<std::string> jackIds;
  for (Status s : {Status::Together, Status::EscapedAlone, Status::Captured, Status::Dead, Status::Unknown}) {
    auto o = base;
    o["jack"] = s;
    jackIds.insert(ResolveEpilogue(Difficulty::Hard, o)["jack"]);
  }
  Check(jackIds.size() == 5, "five distinct Jack epilogues");
  Check(ResolveEpilogue(Difficulty::Punitive, base, true, true)["jack_secret"] == "E_JACK_SECRET_REVEALED",
        "J4 witnessed");
  Check(ResolveEpilogue(Difficulty::Punitive, base)["jack_secret"] == "E_JACK_SECRET_NOT_WITNESSED",
        "J4 missed");
  Check(ResolveEpilogue(Difficulty::Hard, base)["jack_secret"] == "E_JACK_BASELINE", "baseline Jack");
  Check(IsKnownNode("B1_MORGUE") && NodeRole("X_ROOF") == "saida", "graph nodes");
  Check(SceneAct("N34") == 3 && SceneAct("N01") == 1, "scene acts");
}

} // namespace

int main() {
  try {
    EarlyScenes();
    LateScenes();
    Endings();
    Variants();
    std::cout << "NarrativeCore: " << assertions << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "NarrativeCore test failed after " << assertions << " checks: " << error.what() << '\n';
    return 1;
  }
}
