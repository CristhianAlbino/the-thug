#pragma once
#include "Authoring.h"
#include "../SaveCore/NarrativeState.h"
#include <cstdint>
#include <optional>

namespace thug::director {
struct Actor { std::string node; bool active = true; };
struct Observation {
  rules::Difficulty difficulty = rules::Difficulty::Normal;
  int act = 1;
  std::string joeNode;
  std::map<std::string, Actor> companions;
  std::set<std::string> facts;
  bool hunterHasEvidence = false, hunterCanReach = false,
       alternativeReachable = false, areaPreviouslyVisited = false;
  std::string routeFailure;
  bool inDanger = false;
  double nowSeconds = 0;
  bool sightAndSoundVerified = false;
};
// Selected cue IDs and activation serials are durable. They are separate from
// journal facts: a suspended presentation does not prove its undelivered lines.
struct PresentationState {
  std::uint64_t serial = 0, sceneSerial = 0, banterSerial = 0;
  std::vector<std::string> scenePlan, banterPlan;
};
struct Presentation {
  std::string owner;
  std::uint64_t serial;
  int index;
  Cue cue;
};

const Catalogue& Content();
bool ValidObservation(const Observation&);
bool SceneEligible(const std::string&, const Observation&, const save::NarrativeSnapshot&);
bool BanterEligible(const std::string&, const Observation&, const save::NarrativeSnapshot&);
bool BanterCanResume(const Observation&, const save::NarrativeSnapshot&);
bool CanWitness(const std::string&, const Observation&, const save::NarrativeSnapshot&);
bool VariantAvailable(const std::string&, rules::Difficulty, const std::set<std::string>& clues, int act);
bool InspectAllowed(const std::string&, rules::Difficulty);
bool PersistentFact(const std::string&);
bool ValidPresentation(const save::NarrativeSnapshot&, const PresentationState&);
bool BeginScene(const std::string&, const Observation&, save::NarrativeSnapshot&, PresentationState&);
bool BeginBanter(const std::string&, const Observation&, save::NarrativeSnapshot&, PresentationState&);
bool CancelScene(save::NarrativeSnapshot&, PresentationState&);
bool CancelBanter(save::NarrativeSnapshot&, PresentationState&);
std::optional<Presentation> Current(const Observation&, const save::NarrativeSnapshot&, const PresentationState&, const std::string& locale = "pt-BR");
bool Acknowledge(const Presentation&, const Observation&, save::NarrativeSnapshot&, PresentationState&);
std::map<std::string, std::string> ResolveEpilogue(const save::NarrativeSnapshot&);
std::vector<Cue> EpilogueText(const save::NarrativeSnapshot&, const std::string& locale = "pt-BR");
std::vector<Cue> InspectedText(const std::string&, const save::NarrativeSnapshot&, const std::string& locale = "pt-BR");
std::vector<std::pair<std::string, std::string>> JournalFragments(const save::NarrativeSnapshot&);
} // namespace thug::director
