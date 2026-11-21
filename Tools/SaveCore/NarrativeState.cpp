#include "NarrativeState.h"
#include "NarrativeFacts.generated.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace thug::save {

void swap(NarrativeSnapshot& a, NarrativeSnapshot& b) noexcept {
  // Member swaps exchange owned buffers/nodes, without the potentially
  // allocating move-construction used by generic aggregate std::swap on MSVC.
  static_assert(noexcept(a.seenScenes.swap(b.seenScenes)));
  static_assert(noexcept(a.deliveredBanter.swap(b.deliveredBanter)));
  static_assert(noexcept(a.inspectedDocuments.swap(b.inspectedDocuments)));
  static_assert(noexcept(a.inspectedAudio.swap(b.inspectedAudio)));
  static_assert(noexcept(a.witnessedClues.swap(b.witnessedClues)));
  static_assert(noexcept(a.durableFacts.swap(b.durableFacts)));
  static_assert(noexcept(a.activeScene.swap(b.activeScene)));
  static_assert(noexcept(a.activeBanter.swap(b.activeBanter)));
  static_assert(noexcept(a.outcomes.swap(b.outcomes)));
  using std::swap;
  swap(a.difficulty, b.difficulty);
  swap(a.nowSeconds, b.nowSeconds);
  swap(a.act, b.act);
  a.seenScenes.swap(b.seenScenes);
  a.deliveredBanter.swap(b.deliveredBanter);
  a.inspectedDocuments.swap(b.inspectedDocuments);
  a.inspectedAudio.swap(b.inspectedAudio);
  a.witnessedClues.swap(b.witnessedClues);
  a.durableFacts.swap(b.durableFacts);
  a.activeScene.swap(b.activeScene);
  swap(a.activeSceneLine, b.activeSceneLine);
  a.activeBanter.swap(b.activeBanter);
  swap(a.activeBanterLine, b.activeBanterLine);
  swap(a.lastBanterCompletedAt, b.lastBanterCompletedAt);
  swap(a.keypadAttempts, b.keypadAttempts);
  swap(a.joeEscaped, b.joeEscaped);
  a.outcomes.swap(b.outcomes);
}
namespace {

bool Numbered(const std::string& id, const std::string& prefix,
              int low, int high) {
  if (id.size() != prefix.size() + 2 || id.compare(0, prefix.size(), prefix))
    return false;
  const auto a = id[prefix.size()];
  const auto b = id[prefix.size() + 1];
  if (a < '0' || a > '9' || b < '0' || b > '9') return false;
  const int value = (a - '0') * 10 + b - '0';
  return value >= low && value <= high;
}

bool Scene(const std::string& id) {
  if (id == "SP_ROBIN" || id == "SP_ALFRED" || id == "SP_GORDON" || id == "SP_ORACLE" || id == "SP_FLASH") return true;
  if (id == "J4") return true;
  if (Numbered(id, "N", 1, 34)) return true;
  if (id.size() == 8 && id.substr(3) == ":exit" &&
      Numbered(id.substr(0, 3), "N", 28, 31)) return true;
  if (id.rfind("N26:", 0) != 0) return false;
  const auto route = id.substr(4);
  return route == "garage" || route == "roof" || route == "tunnel" ||
         route == "window";
}

bool Clue(const std::string& id) {
  return id == "J1" || id == "J2" || id == "J3" || id == "J4";
}

bool DurableFact(const std::string& id) {
  if (id.empty() || id.size() > 1024) return false;
  for (unsigned char c : id)
    if (c < 33 || c > 126) return false;
  return true;
}

bool PersistentFact(const std::string& id) {
  // These are current encounter observations, never journal knowledge.
  return DurableFact(id) && !Clue(id) && !data::TransientFacts().count(id);
}

template <typename Predicate>
bool ValidSet(const std::set<std::string>& ids, Predicate predicate) {
  if (ids.size() > 10000) return false;
  for (const auto& id : ids)
    if (!predicate(id)) return false;
  return true;
}

bool Valid(const NarrativeSnapshot& s) {
  const int difficulty = static_cast<int>(s.difficulty);
  if (difficulty < 0 || difficulty > 3 ||
      !std::isfinite(s.nowSeconds) || s.nowSeconds < 0 ||
      s.act < 1 || s.act > 5 || s.keypadAttempts > 100000 ||
      !std::isfinite(s.lastBanterCompletedAt) ||
      (s.lastBanterCompletedAt < 0 && s.lastBanterCompletedAt != -1) ||
      s.lastBanterCompletedAt > s.nowSeconds ||
      !ValidSet(s.seenScenes, Scene) ||
      !ValidSet(s.deliveredBanter, [](const std::string& id) {
        return Numbered(id, "BAN", 1, 42);
      }) ||
      !ValidSet(s.inspectedDocuments, [](const std::string& id) {
        return Numbered(id, "DOC", 1, 16);
      }) ||
      !ValidSet(s.inspectedAudio, [](const std::string& id) {
        return Numbered(id, "AUD", 1, 8);
      }) ||
      !ValidSet(s.witnessedClues, Clue) ||
      !ValidSet(s.durableFacts, PersistentFact) ||
      (s.activeScene.empty() ? s.activeSceneLine != 0 :
       !Scene(s.activeScene) || s.activeSceneLine < 0 ||
       s.activeSceneLine > 10000 || s.seenScenes.count(s.activeScene)) ||
      (s.activeBanter.empty() ? s.activeBanterLine != 0 :
       !Numbered(s.activeBanter, "BAN", 1, 42) ||
       s.activeBanterLine < 0 || s.activeBanterLine > 10000 ||
       s.deliveredBanter.count(s.activeBanter)) ||
      s.outcomes.size() != 3) return false;
  if (difficulty < 2 && (s.seenScenes.count("N34") ||
      s.activeScene == "N34" || s.inspectedDocuments.count("DOC16") ||
      s.inspectedAudio.count("AUD08") ||
      s.durableFacts.count("joe_identified_thomas_body") ||
      s.durableFacts.count("joe_verified_batman_record") ||
      s.durableFacts.count("joe_directly_witnessed_thomas_death"))) return false;
  if (difficulty != 3 && s.inspectedAudio.count("AUD07")) return false;
  if (difficulty != 3 && (s.seenScenes.count("SP_ROBIN") || s.activeScene == "SP_ROBIN")) return false;
  if ((s.activeScene == "N33" || s.seenScenes.count("N33")) &&
      !((difficulty == 0 && s.joeEscaped && s.durableFacts.count("rare_confrontation")) ||
        (difficulty == 3 && !s.joeEscaped && s.durableFacts.count("rare_destruction")))) return false;
  if (!s.joeEscaped) {
    for (const auto& id : {"N28:exit", "N29:exit", "N30:exit", "N31:exit"})
      if (s.activeScene == id || s.seenScenes.count(id)) return false;
  }
  if (difficulty != 3 && !s.witnessedClues.empty()) return false;
  if ((s.activeScene == "J4" || s.seenScenes.count("J4")) &&
      (difficulty != 3 || s.act != 5 || !s.witnessedClues.count("J3") || s.witnessedClues.size() < 2)) return false;
  for (const auto& clue : s.witnessedClues)
    if (s.act < clue[1] - '0' + 1) return false;
  if (s.witnessedClues.count("J4") &&
      (!s.witnessedClues.count("J3") || s.witnessedClues.size() < 3))
    return false;
  for (const auto& name : {"harley", "bartholomeu", "jack"}) {
    const auto it = s.outcomes.find(name);
    if (it == s.outcomes.end()) return false;
    const auto& o = it->second;
    if (int(o.together) + int(o.escapedAlone) + int(o.captured) +
            int(o.dead) > 1 ||
        (o.together && !s.joeEscaped) ||
        (!o.lastContactNode.empty() && !DurableFact(o.lastContactNode)))
      return false;
  }
  return true;
}

} // namespace

NarrativeState::NarrativeState(rules::Difficulty difficulty) {
  if (static_cast<int>(difficulty) < 0 || static_cast<int>(difficulty) > 3)
    throw std::invalid_argument("Invalid narrative difficulty");
  snapshot_.difficulty = difficulty;
}

bool NarrativeState::RestoreSnapshot(const NarrativeSnapshot& snapshot) {
  if (snapshot.difficulty != snapshot_.difficulty || !Valid(snapshot)) return false;
  auto staged = snapshot;
  swap(snapshot_, staged);
  return true;
}

bool NarrativeState::RecordScene(const std::string& id, double atSeconds) {
  auto next = snapshot_;
  if (!Scene(id) || !std::isfinite(atSeconds) ||
      atSeconds < next.nowSeconds || next.seenScenes.count(id)) return false;
  next.nowSeconds = atSeconds;
  next.seenScenes.insert(id);
  if (next.activeScene == id) {
    next.activeScene.clear();
    next.activeSceneLine = 0;
  }
  return RestoreSnapshot(next);
}

bool NarrativeState::RecordBanter(const std::string& id, double atSeconds) {
  auto next = snapshot_;
  if (!Numbered(id, "BAN", 1, 42) || !std::isfinite(atSeconds) ||
      atSeconds < next.nowSeconds || next.deliveredBanter.count(id)) return false;
  next.nowSeconds = atSeconds;
  next.lastBanterCompletedAt = atSeconds;
  next.deliveredBanter.insert(id);
  if (next.activeBanter == id) {
    next.activeBanter.clear();
    next.activeBanterLine = 0;
  }
  return RestoreSnapshot(next);
}

bool NarrativeState::InspectDocument(const std::string& id, double atSeconds) {
  auto next = snapshot_;
  if (!Numbered(id, "DOC", 1, 16) || !std::isfinite(atSeconds) ||
      atSeconds < next.nowSeconds || next.inspectedDocuments.count(id))
    return false;
  next.nowSeconds = atSeconds;
  next.inspectedDocuments.insert(id);
  return RestoreSnapshot(next);
}

bool NarrativeState::InspectAudio(const std::string& id, double atSeconds) {
  auto next = snapshot_;
  if (!Numbered(id, "AUD", 1, 8) || !std::isfinite(atSeconds) ||
      atSeconds < next.nowSeconds || next.inspectedAudio.count(id)) return false;
  next.nowSeconds = atSeconds;
  next.inspectedAudio.insert(id);
  return RestoreSnapshot(next);
}

bool NarrativeState::WitnessClue(const std::string& id, double atSeconds) {
  auto next = snapshot_;
  if (!Clue(id) || !std::isfinite(atSeconds) ||
      atSeconds < next.nowSeconds || next.witnessedClues.count(id)) return false;
  next.nowSeconds = atSeconds;
  next.witnessedClues.insert(id);
  return RestoreSnapshot(next);
}

void swap(NarrativeState& a, NarrativeState& b) noexcept {
  using std::swap;
  swap(a.snapshot_, b.snapshot_);
}

} // namespace thug::save
