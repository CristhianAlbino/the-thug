#include "NarrativeCore.h"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace thug::narrative {

namespace {

struct SceneDef {
  const char* id;
  int act;
  std::vector<std::string> anchors;
  std::vector<std::string> mustSeen;
  std::vector<std::string> anySeen;
  const char* alternativeFact;
  const char* companion;
  std::vector<std::string> factsAny;
  std::vector<std::string> difficulty;
  bool requiresHunterEvidence;
  bool requiresReachableHunter;
  bool requiresReachableExit;
  bool requiresVisitedArea;
  bool late;
};

struct NodeDef {
  const char* id;
  const char* role;
};

struct EpilogueDef {
  const char* person;
  const char* status;
  const char* asymmetric;
  const char* reciprocal;
};

#include "NarrativeData.inl"

const char* const kDifficultyNames[] = {"Easy", "Normal", "Hard", "Punitive"};
const char* const kPeople[] = {"harley", "bartholomeu", "jack"};

const SceneDef* FindScene(const std::string& id) {
  for (const SceneDef& scene : kScenes) {
    if (id == scene.id) return &scene;
  }
  return nullptr;
}

const SceneDef& SceneOrThrow(const std::string& id) {
  if (const SceneDef* scene = FindScene(id)) return *scene;
  throw std::invalid_argument("unknown scene: " + id);
}

bool Has(const std::set<std::string>& set, const std::string& value) {
  return set.count(value) != 0;
}

bool Contains(const std::vector<std::string>& list, const std::string& value) {
  return std::find(list.begin(), list.end(), value) != list.end();
}

bool AnyIn(const std::vector<std::string>& list, const std::set<std::string>& set) {
  for (const std::string& item : list) {
    if (Has(set, item)) return true;
  }
  return false;
}

bool CompanionHere(const Facts& state, const std::string& who) {
  auto it = state.companions.find(who);
  return it != state.companions.end() && it->second.active &&
         it->second.node == state.joeNode;
}

bool IsJackSecretId(const std::string& id) {
  if (id == kJackReveal) return true;
  for (const char* clue : kJackClues) {
    if (id == clue) return true;
  }
  return false;
}

int MinimumAct(const std::string& id) {
  for (const auto& entry : kJackMinimumAct) {
    if (id == entry.first) return entry.second;
  }
  return 1;
}

bool EligibleEarly(const SceneDef& scene, const Facts& state) {
  if (Has(state.seen, scene.id)) return false;
  for (const std::string& required : scene.mustSeen) {
    if (!Has(state.seen, required)) return false;
  }
  if (!scene.anchors.empty() && !Contains(scene.anchors, state.joeNode)) return false;
  if (*scene.companion && !CompanionHere(state, scene.companion)) return false;
  if (!scene.anySeen.empty() && !AnyIn(scene.anySeen, state.seen) &&
      !Has(state.facts, scene.alternativeFact)) {
    return false;
  }
  if (scene.requiresHunterEvidence && !state.hunterHasEvidence) return false;
  if (scene.requiresReachableHunter && !state.hunterCanReach) return false;
  if (scene.requiresReachableExit && !state.alternativeReachable) return false;
  if (scene.requiresVisitedArea && !state.areaPreviouslyVisited) return false;
  return true;
}

bool EligibleLate(const SceneDef& scene, const Facts& state) {
  const std::string id = scene.id;
  if (!Has(state.seen, "N03") || (id != "N26" && Has(state.seen, id))) return false;
  if (!IsKnownNode(state.joeNode)) return false;
  const std::string role = NodeRole(state.joeNode);
  if (id == "N21" && role != "circulacao") return false;
  if (id == "N26" && role == "saida") return false;
  if (!scene.anchors.empty() && !Contains(scene.anchors, state.joeNode)) return false;
  if (!scene.difficulty.empty() &&
      !Contains(scene.difficulty, kDifficultyNames[static_cast<int>(state.difficulty)])) {
    return false;
  }
  if (*scene.companion && !CompanionHere(state, scene.companion)) return false;
  if (!scene.factsAny.empty() && !AnyIn(scene.factsAny, state.facts)) return false;
  if (scene.requiresReachableHunter && !state.hunterCanReach) return false;
  if (scene.requiresReachableExit && !state.alternativeReachable) return false;
  if (id == "N27" && state.joeNode == "F3_ISOLATION" && !Has(state.facts, "isolation_known")) {
    return false;
  }
  if (id == "N26") {
    const std::string& r = state.routeFailure;
    if ((r != "garage" && r != "roof" && r != "tunnel" && r != "window") ||
        Has(state.seen, "N26:" + r) || !Has(state.facts, "route_failure_observed") ||
        !Has(state.facts, "route_failed:" + r)) {
      return false;
    }
  }
  if (id == "N32" && state.difficulty == Difficulty::Punitive && !Has(state.facts, "hide_victory")) {
    return false;
  }
  if (id == "N33") {
    const char* winning = state.difficulty == Difficulty::Easy ? "rare_confrontation"
                          : state.difficulty == Difficulty::Punitive ? "rare_destruction"
                                                                      : nullptr;
    if (!winning || !Has(state.facts, winning)) return false;
  }
  if (id == "N34" &&
      !((Has(state.facts, "joe_identified_thomas_body") &&
         Has(state.facts, "joe_verified_batman_record")) ||
        Has(state.facts, "joe_directly_witnessed_thomas_death"))) {
    return false;
  }
  return true;
}

} // namespace

const Profile& ProfileFor(Difficulty difficulty) {
  const int index = static_cast<int>(difficulty);
  if (index < 0 || index > 3) throw std::invalid_argument("unknown difficulty");
  return kProfiles[index];
}

const char* StatusName(Status status) {
  switch (status) {
  case Status::Together: return "together";
  case Status::EscapedAlone: return "escaped_alone";
  case Status::Captured: return "captured";
  case Status::Dead: return "dead";
  case Status::Unknown: return "unknown";
  }
  throw std::invalid_argument("unknown status");
}

bool IsKnownScene(const std::string& sceneId) { return FindScene(sceneId) != nullptr; }

int SceneAct(const std::string& sceneId) { return SceneOrThrow(sceneId).act; }

bool IsKnownNode(const std::string& node) {
  for (const NodeDef& def : kNodes) {
    if (node == def.id) return true;
  }
  return false;
}

std::string NodeRole(const std::string& node) {
  for (const NodeDef& def : kNodes) {
    if (node == def.id) return def.role;
  }
  return std::string();
}

bool SceneAvailable(const std::string& sceneId, Difficulty difficulty,
                    const std::set<std::string>& witnessedJackClues, bool jackAvailable,
                    int act) {
  const Profile& profile = ProfileFor(difficulty);
  if (sceneId == kThomasScene) return profile.thomasConfirmed;
  if (IsJackSecretId(sceneId)) {
    if (!profile.jackSecret || !jackAvailable) return false;
    if (act < MinimumAct(sceneId)) return false;
    if (sceneId == kJackReveal) {
      int seen = 0;
      for (const char* clue : kJackClues) {
        if (Has(witnessedJackClues, clue)) ++seen;
      }
      return Has(witnessedJackClues, kJackLateClue) && seen >= kJackMinimumClues;
    }
  }
  return true;
}

bool Eligible(const std::string& sceneId, const Facts& state) {
  ProfileFor(state.difficulty);  // reject unknown modes instead of leaking content
  const SceneDef& scene = SceneOrThrow(sceneId);
  return scene.late ? EligibleLate(scene, state) : EligibleEarly(scene, state);
}

bool CanWitnessJ1(const Facts& state, bool sightAndSound) {
  return Has(state.seen, "N08") && sightAndSound && CompanionHere(state, "jack") &&
         SceneAvailable("J1", state.difficulty, {}, true, 2);
}

bool CanWitnessLateClue(const std::string& clueId, const Facts& state, int act,
                        bool sightAndSound) {
  if (clueId != "J2" && clueId != "J3" && clueId != "J4") {
    throw std::invalid_argument("only late Jack clues are handled here");
  }
  if (!sightAndSound || !CompanionHere(state, "jack")) return false;
  if (clueId == "J3" && !Has(state.seen, "N25")) return false;
  if (clueId == "J2" && !(Has(state.facts, "jack_act3_encounter") &&
                          Has(state.facts, "jack_heard_thomas_or_injury"))) {
    return false;
  }
  if (clueId == "J3" && !Has(state.facts, "jack_perceived_companion_risk")) return false;
  if (clueId == "J4" && !Has(state.facts, "jack_final_encounter")) return false;
  std::set<std::string> seenClues;
  for (const char* clue : kJackClues) {
    if (Has(state.facts, clue)) seenClues.insert(clue);
  }
  return SceneAvailable(clueId, state.difficulty, seenClues, true, act);
}

bool CanCloseRoute(const std::string& sceneId, const Facts& state) {
  static const std::map<std::string, std::pair<std::string, std::string>> kRouteEnds = {
      {"N28", {"X_GARAGE", "garage_escaped"}},
      {"N29", {"X_ROOF", "roof_escaped"}},
      {"N30", {"X_TUNNEL", "tunnel_escaped"}},
      {"N31", {"X_WINDOW", "window_escaped"}},
  };
  auto it = kRouteEnds.find(sceneId);
  if (it == kRouteEnds.end()) throw std::invalid_argument("not a route ending scene: " + sceneId);
  return state.joeNode == it->second.first && Has(state.facts, it->second.second) &&
         Has(state.seen, "N03");
}

std::map<std::string, Status> DeriveOutcomes(
    const std::map<std::string, CompanionEvidence>& evidence, bool joeEscaped) {
  if (evidence.size() != 3) throw std::invalid_argument("one evidence record per companion");
  std::map<std::string, Status> outcomes;
  for (const char* person : kPeople) {
    auto it = evidence.find(person);
    if (it == evidence.end()) throw std::invalid_argument("one evidence record per companion");
    const CompanionEvidence& record = it->second;
    const int verified = record.togetherAtJoeExit + record.separateEscapeVerified +
                         record.captureVerified + record.deathVerified;
    if (verified > 1) {
      throw std::invalid_argument(std::string("contradictory verified outcomes for ") + person);
    }
    if (record.togetherAtJoeExit && !joeEscaped) {
      throw std::invalid_argument(std::string(person) + " cannot escape together if Joe did not");
    }
    outcomes[person] = record.togetherAtJoeExit        ? Status::Together
                       : record.separateEscapeVerified ? Status::EscapedAlone
                       : record.captureVerified        ? Status::Captured
                       : record.deathVerified          ? Status::Dead
                                                       : Status::Unknown;
  }
  return outcomes;
}

std::map<std::string, std::string> ResolveEpilogue(
    Difficulty difficulty, const std::map<std::string, Status>& outcomes,
    bool thomasSceneWitnessed, bool jackRevelationWitnessed) {
  const Profile& profile = ProfileFor(difficulty);
  if (outcomes.size() != 3) throw std::invalid_argument("one outcome is required for each companion");
  if (thomasSceneWitnessed && !profile.thomasConfirmed) {
    throw std::invalid_argument("Thomas reveal cannot be witnessed in this difficulty");
  }
  if (jackRevelationWitnessed && !profile.jackSecret) {
    throw std::invalid_argument("Jack's secret cannot be witnessed in this difficulty");
  }
  std::map<std::string, std::string> result;
  for (const char* person : kPeople) {
    auto it = outcomes.find(person);
    if (it == outcomes.end()) throw std::invalid_argument("one outcome is required for each companion");
    const std::string status = StatusName(it->second);
    for (const EpilogueDef& def : kEpilogues) {
      if (status == def.status && std::string(person) == def.person) {
        result[person] = profile.harleyReciprocal ? def.reciprocal : def.asymmetric;
      }
    }
  }
  result["thomas"] = !profile.thomasConfirmed ? kThomasUnconfirmed
                     : thomasSceneWitnessed   ? kThomasJoeKnows
                                              : kThomasFallback;
  result["jack_secret"] = jackRevelationWitnessed ? "E_JACK_SECRET_REVEALED"
                          : profile.jackSecret    ? "E_JACK_SECRET_NOT_WITNESSED"
                                                  : "E_JACK_BASELINE";
  return result;
}

} // namespace thug::narrative
