#pragma once

#include "../HunterCore/RunRules.h"

#include <map>
#include <set>
#include <string>

namespace thug::save {

// Only witnessed, durable facts belong here. Current encounter conditions stay
// in the scene adapter and expire when Joe leaves the encounter.
struct OutcomeEvidence {
  bool together = false;
  bool escapedAlone = false;
  bool captured = false;
  bool dead = false;
  std::string lastContactNode;
};

struct NarrativeSnapshot {
  rules::Difficulty difficulty = rules::Difficulty::Normal;
  double nowSeconds = 0;
  int act = 1;
  std::set<std::string> seenScenes;
  std::set<std::string> deliveredBanter;
  std::set<std::string> inspectedDocuments;
  std::set<std::string> inspectedAudio;
  std::set<std::string> witnessedClues;
  std::set<std::string> durableFacts;
  std::string activeScene;
  int activeSceneLine = 0;
  std::string activeBanter;
  int activeBanterLine = 0;
  double lastBanterCompletedAt = -1;
  unsigned keypadAttempts = 0;
  bool joeEscaped = false;
  std::map<std::string, OutcomeEvidence> outcomes{
      {"harley", {}}, {"bartholomeu", {}}, {"jack", {}}};
};

class NarrativeState {
 public:
  explicit NarrativeState(rules::Difficulty difficulty);
  rules::Difficulty DifficultyLevel() const { return snapshot_.difficulty; }
  NarrativeSnapshot CaptureSnapshot() const { return snapshot_; }
  // Rejected snapshots leave the entire narrative state untouched.
  bool RestoreSnapshot(const NarrativeSnapshot& snapshot);
  bool RecordScene(const std::string& id, double atSeconds);
  bool RecordBanter(const std::string& id, double atSeconds);
  bool InspectDocument(const std::string& id, double atSeconds);
  bool InspectAudio(const std::string& id, double atSeconds);
  bool WitnessClue(const std::string& id, double atSeconds);
  friend void swap(NarrativeState& a, NarrativeState& b) noexcept;

 private:
  NarrativeSnapshot snapshot_;
};

void swap(NarrativeState& a, NarrativeState& b) noexcept;

} // namespace thug::save
