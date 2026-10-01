#include "DirectorCore.h"
#include "DirectorData.generated.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace thug::director {
namespace {
bool Mode(unsigned bits, rules::Difficulty d) {
  const auto n = static_cast<int>(d);
  return n >= 0 && n < 4 && (bits & (1u << n));
}
bool Includes(const std::set<std::string>& values, const std::set<std::string>& needed) {
  return std::includes(values.begin(), values.end(), needed.begin(), needed.end());
}
bool Intersects(const std::set<std::string>& a, const std::set<std::string>& b) {
  for (const auto& value : a) if (b.count(value)) return true;
  return false;
}
std::set<std::string> Facts(const Observation& o, const save::NarrativeSnapshot& s, bool clues = true) {
  auto facts = s.durableFacts;
  facts.insert(o.facts.begin(), o.facts.end());
  if (clues) facts.insert(s.witnessedClues.begin(), s.witnessedClues.end());
  return facts;
}
bool Present(const std::string& actor, const Observation& o) {
  if (actor == "joe" || actor == "radio" || actor == "recording" || actor.empty()) return true;
  const auto found = o.companions.find(actor);
  return found != o.companions.end() && found->second.active && found->second.node == o.joeNode;
}
bool CueEligible(const Cue& c, const Observation& o, const save::NarrativeSnapshot& s) {
  const auto facts = Facts(o, s);
  if (!Mode(c.modes, o.difficulty) || !Includes(facts, c.all) || Intersects(facts, c.none)) return false;
  if (c.kind == CueKind::Speech && !Present(c.speaker, o)) return false;
  for (const auto& actor : c.actors) if (!Present(actor, o)) return false;
  return true;
}
std::string BaseScene(const std::string& id) {
  return id.rfind("N26:", 0) == 0 ? "N26" : id;
}
std::vector<std::string> Select(const std::string& owner, const Observation& o, const save::NarrativeSnapshot& s) {
  std::vector<std::string> plan;
  const auto found = Content().blocks.find(owner);
  if (found == Content().blocks.end() || !Mode(found->second.modes, o.difficulty)) return plan;
  for (const auto& id : found->second.cues) if (CueEligible(Content().cues.at(id), o, s)) plan.push_back(id);
  return plan;
}
bool BanterContext(const std::string& id, const Observation& o, const save::NarrativeSnapshot& s) {
  const auto found = Content().banter.find(id);
  if (found == Content().banter.end()) return false;
  const auto& b = found->second;
  if (!ValidObservation(o) || b.act != o.act || !b.nodes.count(o.joeNode) || s.deliveredBanter.count(id) ||
      o.inDanger || !s.activeScene.empty() || !Mode(b.modes, o.difficulty) ||
      !Includes(s.seenScenes, b.after) || Intersects(s.seenScenes, b.before) ||
      !Includes(Facts(o, s, false), b.requirements)) return false;
  for (const auto& actor : b.actors) if (!Present(actor, o)) return false;
  return true;
}
const std::map<std::string, std::pair<std::string, std::string>> exits{
    {"N28:exit", {"X_GARAGE", "garage_escaped"}}, {"N29:exit", {"X_ROOF", "roof_escaped"}},
    {"N30:exit", {"X_TUNNEL", "tunnel_escaped"}}, {"N31:exit", {"X_WINDOW", "window_escaped"}}};
bool ActiveSceneContext(const Observation& o, const save::NarrativeSnapshot& s) {
  auto available = s;
  available.activeScene.clear();
  return SceneEligible(s.activeScene, o, available);
}
}

const Catalogue& Content() { return data::Content(); }
bool ValidObservation(const Observation& o) {
  const auto mode = static_cast<int>(o.difficulty);
  return mode >= 0 && mode < 4 && o.act >= 1 && o.act <= 5 && std::isfinite(o.nowSeconds) && o.nowSeconds >= 0;
}
bool VariantAvailable(const std::string& id, rules::Difficulty d, const std::set<std::string>& clues, int act) {
  if (static_cast<int>(d) < 0 || static_cast<int>(d) > 3) return false;
  const auto& p = Content().profiles[static_cast<int>(d)];
  if (id == "N34") return p.thomasConfirmed;
  if (id.size() == 2 && id[0] == 'J' && id[1] >= '1' && id[1] <= '4') {
    if (!p.jackSecret || act < Content().clueMinimumAct[id[1] - '1']) return false;
    if (id == "J4") {
      int count = 0;
      for (const auto& clue : {"J1", "J2", "J3"}) count += int(clues.count(clue));
      return clues.count("J3") && count >= Content().minimumClues;
    }
  }
  return true;
}

bool SceneEligible(const std::string& requested, const Observation& o, const save::NarrativeSnapshot& s) {
  if (!ValidObservation(o) || !s.activeScene.empty() || s.seenScenes.count(requested)) return false;
  const auto facts = Facts(o, s);
  const auto coda = exits.find(requested);
  if (coda != exits.end()) return o.joeNode == coda->second.first && facts.count(coda->second.second) && s.seenScenes.count("N03");
  if (requested == "J4") return !s.witnessedClues.count("J4") && o.alternativeReachable && CanWitness("J4", o, s);
  const auto id = BaseScene(requested);
  const auto found = Content().scenes.find(id);
  if (found == Content().scenes.end() || !VariantAvailable(id, o.difficulty, s.witnessedClues, o.act)) return false;
  const auto& scene = found->second;
  if ((!scene.late && (s.seenScenes.count(id) || !Includes(s.seenScenes, scene.mustSeen))) ||
      (scene.late && (!s.seenScenes.count("N03") || (id != "N26" && s.seenScenes.count(id)))) ||
      (!scene.anchors.empty() && !scene.anchors.count(o.joeNode)) || !Mode(scene.modes, o.difficulty) ||
      (!scene.companion.empty() && !Present(scene.companion, o)) ||
      (!scene.anySeen.empty() && !Intersects(scene.anySeen, s.seenScenes) && !facts.count(scene.alternativeFact)) ||
      (!scene.factsAny.empty() && !Intersects(scene.factsAny, facts)) ||
      (scene.hunterEvidence && !o.hunterHasEvidence) || (scene.hunterReachable && !o.hunterCanReach) ||
      (scene.exitReachable && !o.alternativeReachable) || (scene.previouslyVisited && !o.areaPreviouslyVisited)) return false;
  const auto node = Content().nodeRoles.find(o.joeNode);
  if (scene.late && node == Content().nodeRoles.end()) return false;
  if (id == "N21" && node->second != "circulacao") return false;
  if (id == "N26") {
    const auto complete = "N26:" + o.routeFailure;
    if (node->second == "saida" || (o.routeFailure != "garage" && o.routeFailure != "roof" &&
        o.routeFailure != "tunnel" && o.routeFailure != "window") || s.seenScenes.count(complete) ||
        !facts.count("route_failure_observed") || !facts.count("route_failed:" + o.routeFailure) ||
        (requested != "N26" && requested != complete)) return false;
  }
  if (id == "N27" && o.joeNode == "F3_ISOLATION" && !facts.count("isolation_known")) return false;
  if (id == "N32" && o.difficulty == rules::Difficulty::Punitive && !facts.count("hide_victory")) return false;
  if (id == "N33") {
    const auto winning = o.difficulty == rules::Difficulty::Easy ? "rare_confrontation" :
                         o.difficulty == rules::Difficulty::Punitive ? "rare_destruction" : "";
    if (!facts.count(winning)) return false;
  }
  if (id == "N34" && !(facts.count("joe_directly_witnessed_thomas_death") ||
      (facts.count("joe_identified_thomas_body") && facts.count("joe_verified_batman_record")))) return false;
  for (const auto& [codaId, exit] : exits) if (codaId.substr(0, 3) == id && o.joeNode == exit.first) return false;
  return true;
}

bool BanterEligible(const std::string& id, const Observation& o, const save::NarrativeSnapshot& s) {
  return s.activeScene.empty() && s.activeBanter.empty() &&
      (s.lastBanterCompletedAt < 0 || o.nowSeconds - s.lastBanterCompletedAt >= 90) && BanterContext(id, o, s);
}
bool BanterCanResume(const Observation& o, const save::NarrativeSnapshot& s) {
  return !s.activeBanter.empty() && o.nowSeconds >= s.lastBanterCompletedAt && BanterContext(s.activeBanter, o, s);
}
bool CanWitness(const std::string& id, const Observation& o, const save::NarrativeSnapshot& s) {
  if (!ValidObservation(o) || !o.sightAndSoundVerified || !Present("jack", o) ||
      !VariantAvailable(id, o.difficulty, s.witnessedClues, o.act)) return false;
  const auto facts = Facts(o, s);
  if (id == "J1") return s.seenScenes.count("N08");
  if (id == "J2") return facts.count("jack_act3_encounter") && facts.count("jack_heard_thomas_or_injury");
  if (id == "J3") return s.seenScenes.count("N25") && facts.count("jack_perceived_companion_risk");
  if (id == "J4") return facts.count("jack_final_encounter");
  return false;
}
bool InspectAllowed(const std::string& id, rules::Difficulty d) {
  const auto found = Content().blocks.find(id);
  return found != Content().blocks.end() && Mode(found->second.modes, d) &&
      (id.rfind("DOC", 0) == 0 || id.rfind("AUD", 0) == 0);
}
bool PersistentFact(const std::string& id) {
  if (id.empty() || id.size() > 128 || !Content().persistentFacts.count(id) || Content().transientFacts.count(id) || (id.size() == 2 && id[0] == 'J')) return false;
  for (unsigned char c : id) if (c < 33 || c > 126) return false;
  return true;
}

bool ValidPresentation(const save::NarrativeSnapshot& s, const PresentationState& p) {
  const auto validate = [&](const std::string& owner, int index, const auto& plan, std::uint64_t serial) {
    if (owner.empty()) return plan.empty() && serial == 0 && index == 0;
    const auto block = Content().blocks.find(owner);
    if (block == Content().blocks.end() || !Mode(block->second.modes, s.difficulty) || plan.empty() ||
        plan.size() > 1000 || index < 0 || static_cast<std::size_t>(index) >= plan.size() || !serial || serial > p.serial) return false;
    std::set<std::string> ids;
    std::size_t last = 0;
    for (const auto& id : plan) {
      const auto cue = Content().cues.find(id);
      if (cue == Content().cues.end() || cue->second.owner != owner || !Mode(cue->second.modes, s.difficulty) || !ids.insert(id).second) return false;
      const auto at = std::find(block->second.cues.begin(), block->second.cues.end(), id);
      if (at == block->second.cues.end() || static_cast<std::size_t>(at - block->second.cues.begin()) < last) return false;
      last = static_cast<std::size_t>(at - block->second.cues.begin()) + 1;
    }
    return true;
  };
  return validate(s.activeScene, s.activeSceneLine, p.scenePlan, p.sceneSerial) &&
      validate(s.activeBanter, s.activeBanterLine, p.banterPlan, p.banterSerial) &&
      (!p.sceneSerial || !p.banterSerial || p.sceneSerial != p.banterSerial);
}
bool BeginScene(const std::string& requested, const Observation& o, save::NarrativeSnapshot& s, PresentationState& p) {
  if (!SceneEligible(requested, o, s) || p.serial == std::numeric_limits<std::uint64_t>::max()) return false;
  const auto id = requested == "N26" ? "N26:" + o.routeFailure : requested;
  const auto plan = Select(id, o, s);
  if (plan.empty()) return false;
  s.activeScene = id; s.activeSceneLine = 0; p.scenePlan = plan; p.sceneSerial = ++p.serial;
  return true;
}
bool BeginBanter(const std::string& id, const Observation& o, save::NarrativeSnapshot& s, PresentationState& p) {
  if (!BanterEligible(id, o, s) || p.serial == std::numeric_limits<std::uint64_t>::max()) return false;
  const auto plan = Select(id, o, s);
  if (plan.empty()) return false;
  s.activeBanter = id; s.activeBanterLine = 0; p.banterPlan = plan; p.banterSerial = ++p.serial;
  return true;
}
bool CancelScene(save::NarrativeSnapshot& s, PresentationState& p) {
  if (s.activeScene.empty()) return false;
  s.activeScene.clear(); s.activeSceneLine = 0; p.scenePlan.clear(); p.sceneSerial = 0; return true;
}
bool CancelBanter(save::NarrativeSnapshot& s, PresentationState& p) {
  if (s.activeBanter.empty()) return false;
  s.activeBanter.clear(); s.activeBanterLine = 0; p.banterPlan.clear(); p.banterSerial = 0; return true;
}
std::optional<Presentation> Current(const Observation& o, const save::NarrativeSnapshot& s, const PresentationState& p, const std::string& locale) {
  if (locale != "pt-BR" || !ValidPresentation(s, p)) return std::nullopt;
  const bool main = !s.activeScene.empty();
  if (main ? !ActiveSceneContext(o, s) : !BanterCanResume(o, s)) return std::nullopt;
  if (main && o.inDanger && BaseScene(s.activeScene) != "N03" && BaseScene(s.activeScene) != "N11") return std::nullopt;
  const auto& plan = main ? p.scenePlan : p.banterPlan;
  const int index = main ? s.activeSceneLine : s.activeBanterLine;
  const auto& cue = Content().cues.at(plan.at(static_cast<std::size_t>(index)));
  if (!CueEligible(cue, o, s)) return std::nullopt;
  return Presentation{main ? s.activeScene : s.activeBanter, main ? p.sceneSerial : p.banterSerial, index, cue};
}
bool Acknowledge(const Presentation& requested, const Observation& o, save::NarrativeSnapshot& s, PresentationState& p) {
  const auto current = Current(o, s, p);
  if (!current || current->owner != requested.owner || current->serial != requested.serial ||
      current->index != requested.index || current->cue.id != requested.cue.id) return false;
  const bool main = !s.activeScene.empty();
  const auto id = current->owner;
  const auto& plan = main ? p.scenePlan : p.banterPlan;
  const bool final = static_cast<std::size_t>(current->index + 1) == plan.size();
  if (id == "J4" && final && !CanWitness("J4", o, s)) return false;
  if (!current->cue.deliveredFact.empty()) s.durableFacts.insert(current->cue.deliveredFact);
  if (!final) { if (main) ++s.activeSceneLine; else ++s.activeBanterLine; return true; }
  if (main) {
    s.seenScenes.insert(id);
    if (id == "J4") s.witnessedClues.insert("J4");
    const auto definition = Content().scenes.find(BaseScene(id));
    if (definition != Content().scenes.end()) s.act = std::max(s.act, definition->second.act);
    CancelScene(s, p);
  } else {
    s.deliveredBanter.insert(id); s.lastBanterCompletedAt = o.nowSeconds; CancelBanter(s, p);
  }
  return true;
}

std::map<std::string, std::string> ResolveEpilogue(const save::NarrativeSnapshot& s) {
  const auto n = static_cast<int>(s.difficulty);
  if (n < 0 || n > 3 || s.outcomes.size() != 3) return {};
  const auto& profile = Content().profiles[n];
  const std::map<std::string, std::string> prefix{{"harley", "E_HARLEY_"}, {"bartholomeu", "E_BARTH_"}, {"jack", "E_JACK_"}};
  std::map<std::string, std::string> result;
  for (const auto& [name, start] : prefix) {
    const auto found = s.outcomes.find(name); if (found == s.outcomes.end()) return {};
    const auto& o = found->second;
    if (int(o.together) + int(o.escapedAlone) + int(o.captured) + int(o.dead) > 1 || (o.together && !s.joeEscaped)) return {};
    std::string ending = o.together ? "TOGETHER" : o.escapedAlone ? "ALONE" : o.captured ? "CAPTURED" : o.dead ? "DEAD" : "UNKNOWN";
    if (name == "harley" && o.together) ending += profile.reciprocal ? "_RECIPROCAL" : "_ASYMMETRIC";
    result[name] = start + ending;
  }
  const bool thomas = s.seenScenes.count("N34"), jack = s.witnessedClues.count("J4");
  if ((thomas && !profile.thomasConfirmed) || (jack && !profile.jackSecret)) return {};
  result["thomas"] = !profile.thomasConfirmed ? "E_THOMAS_UNCONFIRMED" : thomas ? "E_THOMAS_JOE_KNOWS" : "E_THOMAS_CODA_PLAYER_ONLY";
  result["jack_secret"] = jack ? "E_JACK_SECRET_REVEALED" : profile.jackSecret ? "E_JACK_SECRET_NOT_WITNESSED" : "E_JACK_BASELINE";
  return result;
}
std::vector<Cue> EpilogueText(const save::NarrativeSnapshot& s, const std::string& locale) {
  std::vector<Cue> output;
  if (locale != "pt-BR") return output;
  const auto ids = ResolveEpilogue(s); if (ids.empty()) return output;
  const auto append = [&](const std::string& id) { for (const auto& cue : Content().blocks.at(id).cues) output.push_back(Content().cues.at(cue)); };
  if (s.joeEscaped && (s.deliveredBanter.count("BAN12") || s.deliveredBanter.count("BAN13"))) append("E_JOE_WAYNE");
  for (const auto& name : {"harley", "bartholomeu", "jack", "thomas", "jack_secret"}) append(ids.at(name));
  if (s.outcomes.at("harley").together) {
    if (s.durableFacts.count("joe_returned_for_harley")) append("E_HARLEY_RETURNED");
    if (s.durableFacts.count("joe_shared_ammo_with_harley")) append("E_HARLEY_SHARED");
    if (s.durableFacts.count("joe_ignored_harley_request")) append("E_HARLEY_IGNORED");
  }
  if (s.outcomes.at("bartholomeu").together && s.durableFacts.count("bartholomeu_cooperated_n24")) append("E_BARTH_PROTECTED");
  if (s.outcomes.at("jack").together && !s.witnessedClues.count("J4") && s.durableFacts.count("jack_previous_encounter_heard")) append("E_JACK_PAST");
  return output;
}
std::vector<Cue> InspectedText(const std::string& id, const save::NarrativeSnapshot& s, const std::string& locale) {
  std::vector<Cue> result;
  if (locale != "pt-BR" || !InspectAllowed(id, s.difficulty) ||
      (!s.inspectedDocuments.count(id) && !s.inspectedAudio.count(id))) return result;
  for (const auto& cue : Content().blocks.at(id).cues) result.push_back(Content().cues.at(cue));
  return result;
}
std::vector<std::pair<std::string, std::string>> JournalFragments(const save::NarrativeSnapshot& s) {
  std::vector<std::pair<std::string, std::string>> result;
  for (const auto& [id, pair] : Content().journalPairs) if (s.inspectedDocuments.count(id)) result.emplace_back(id, pair);
  return result;
}
} // namespace thug::director
