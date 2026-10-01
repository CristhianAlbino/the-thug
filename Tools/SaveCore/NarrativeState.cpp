#include "NarrativeState.h"

#include <cmath>
#include <utility>

namespace thug::save {
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
  if (Numbered(id, "N", 1, 34)) return true;
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
      s.lastBanterCompletedAt < -1 ||
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
      !ValidSet(s.durableFacts, DurableFact) ||
      (s.activeScene.empty() ? s.activeSceneLine != 0 :
       !Scene(s.activeScene) || s.activeSceneLine < 0 ||
       s.activeSceneLine > 10000 || s.seenScenes.count(s.activeScene)) ||
      (s.activeBanter.empty() ? s.activeBanterLine != 0 :
       !Numbered(s.activeBanter, "BAN", 1, 42) ||
       s.activeBanterLine < 0 || s.activeBanterLine > 10000 ||
       s.deliveredBanter.count(s.activeBanter)) ||
      s.outcomes.size() != 3) return false;
  if (difficulty < 2 && s.seenScenes.count("N34")) return false;
  if (difficulty != 3 && !s.witnessedClues.empty()) return false;
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
  snapshot_.difficulty = difficulty;
}

bool NarrativeState::RestoreSnapshot(const NarrativeSnapshot& snapshot) {
  if (snapshot.difficulty != snapshot_.difficulty || !Valid(snapshot)) return false;
  snapshot_ = snapshot;
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
