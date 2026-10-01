#pragma once
#include "../HunterCore/RunRules.h"
#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace thug::director {
enum class CueKind { Speech, Gesture, Document, Recording, Epilogue };
struct Cue {
  std::string id, owner;
  CueKind kind = CueKind::Speech;
  std::string speaker, text, asset;
  unsigned modes = 15;
  std::set<std::string> all, none, actors;
  std::string deliveredFact;
};
struct Block {
  std::string id;
  unsigned modes = 15;
  std::set<std::string> nodes, requirements;
  std::vector<std::string> cues;
};
struct SceneDefinition {
  std::string id;
  int act = 1;
  std::set<std::string> anchors, mustSeen, anySeen;
  std::string alternativeFact, companion;
  std::set<std::string> factsAny;
  unsigned modes = 15;
  bool late = false, hunterEvidence = false, hunterReachable = false,
       exitReachable = false, previouslyVisited = false;
};
struct BanterDefinition {
  std::string id;
  int act = 1;
  std::set<std::string> nodes, actors, after, before, requirements;
  unsigned modes = 15;
};
struct ModeProfile { bool thomasConfirmed, reciprocal, jackSecret; };
struct Catalogue {
  std::string revision;
  std::string garageCode;
  std::map<std::string, SceneDefinition> scenes;
  std::map<std::string, BanterDefinition> banter;
  std::map<std::string, Block> blocks;
  std::map<std::string, Cue> cues;
  std::map<std::string, std::string> nodeRoles;
  std::set<std::string> transientFacts;
  std::set<std::string> persistentFacts;
  std::array<ModeProfile, 4> profiles;
  std::array<int, 4> clueMinimumAct;
  int minimumClues = 2;
  std::map<std::string, std::string> journalPairs;
};
} // namespace thug::director
