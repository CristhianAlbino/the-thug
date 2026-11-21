#pragma once

// C++17 port of the narrative gates in variant_rules.py, ending_rules.py,
// act12_rules.py and act35_rules.py. The data comes from NarrativeData.inl,
// generated from the JSON contract by emit_cpp.py.
//
// These functions only answer "may this beat play / which epilogue block".
// They never move an actor, witness an event or grant a route: the game
// supplies observed facts and RunRules owns escape results.

#include <map>
#include <set>
#include <string>

namespace thug::narrative {

enum class Difficulty { Easy, Normal, Hard, Punitive };
enum class Status { Together, EscapedAlone, Captured, Dead, Unknown };

struct Profile {
  bool thomasConfirmed = false;   // Hard/Punitive: Batman killed Thomas is shown
  bool harleyReciprocal = false;  // Punitive: Joe cares for Harley as she does
  bool jackSecret = false;        // Punitive: Jack's progressive clues J1-J4
};

struct Actor {
  std::string node;
  bool active = true;
};

struct Facts {
  Difficulty difficulty = Difficulty::Normal;
  std::string joeNode;
  std::set<std::string> seen;
  std::set<std::string> facts;
  std::map<std::string, Actor> companions;
  bool hunterHasEvidence = false;
  bool hunterCanReach = false;
  bool alternativeReachable = false;
  bool areaPreviouslyVisited = false;
  std::string routeFailure;
};

struct CompanionEvidence {
  bool togetherAtJoeExit = false;
  bool separateEscapeVerified = false;
  bool captureVerified = false;
  bool deathVerified = false;
  std::string lastContactNode;
};

const Profile& ProfileFor(Difficulty difficulty);
const char* StatusName(Status status);

bool IsKnownScene(const std::string& sceneId);
int SceneAct(const std::string& sceneId);
bool IsKnownNode(const std::string& node);
std::string NodeRole(const std::string& node);

// variant_rules.scene_available
bool SceneAvailable(const std::string& sceneId, Difficulty difficulty,
                    const std::set<std::string>& witnessedJackClues = {},
                    bool jackAvailable = true, int act = 1);

// act12_rules.eligible for N01-N11, act35_rules.eligible_late for N12-N34.
bool Eligible(const std::string& sceneId, const Facts& state);
bool CanWitnessJ1(const Facts& state, bool sightAndSound);
bool CanWitnessLateClue(const std::string& clueId, const Facts& state, int act,
                        bool sightAndSound);
// N28-N31 may close only after RunRules reported the actual exit.
bool CanCloseRoute(const std::string& sceneId, const Facts& state);

// ending_rules.derive_outcomes: verified facts only; silence is "unknown".
// Throws std::invalid_argument for contradictory or missing records.
std::map<std::string, Status> DeriveOutcomes(
    const std::map<std::string, CompanionEvidence>& evidence, bool joeEscaped);

// variant_rules.resolve_epilogue. Keys: harley, bartholomeu, jack, thomas, jack_secret.
std::map<std::string, std::string> ResolveEpilogue(
    Difficulty difficulty, const std::map<std::string, Status>& outcomes,
    bool thomasSceneWitnessed = false, bool jackRevelationWitnessed = false);

} // namespace thug::narrative
