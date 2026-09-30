#include "PerceptionCore.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace thug;

namespace {

int checks = 0;
void Check(bool value, const char* message) {
  ++checks;
  if (!value) throw std::runtime_error(message);
}

void ApproximateSoundAndHunter() {
  perception::PerceptionCore sensor({}, 17);
  hunter::HunterCore hunter(hunter::Difficulty::Hard);
  const perception::SoundEvent shot{perception::SoundKind::Gunshot,
                                    {20, 0, 0}, 2, 1};
  const perception::ListenerContext listener{{0, 0, 0}, 1, 0};
  auto heard = sensor.Hear(shot, listener);
  Check(heard.has_value(), "Nearby shot is heard");
  Check(heard->kind == hunter::StimulusKind::Sound &&
            heard->subject == hunter::Subject::Unknown,
        "Sound never reveals shooter identity");
  Check(heard->perceivedType == "gunshot", "Sound type is retained");
  Check(heard->estimate->radiusMeters >= 4,
        "Estimate retains required uncertainty");
  Check(heard->estimate->center.x != shot.actualSource.x ||
            heard->estimate->center.y != shot.actualSource.y,
        "Estimated center is not exact source");
  Check(hunter.Observe(*heard), "Hunter accepts perceived sound");
  Check(hunter.CurrentState() == hunter::State::Investigating,
        "Gunshot can trigger investigation");
  Check(hunter.LastHeardType() == "gunshot", "Type reaches Hunter memory");
  Check(hunter.RouteEvidence("west") == 0 &&
            hunter.HideSpotSuspicion("locker") == 0,
        "Audio does not reveal player habits");
}

void DecoyAndMasking() {
  perception::PerceptionCore sensor({}, 9);
  auto distraction = sensor.Hear(
      {perception::SoundKind::Distraction, {5, 2, 0}, 1, 1},
      {{0, 0, 0}, 1, 0});
  Check(distraction.has_value() &&
            distraction->subject == hunter::Subject::Unknown,
        "Decoy can be mistaken for an unknown source");
  Check(!sensor.Hear({perception::SoundKind::Explosion, {1, 0, 0}, 2, 1},
                     {{0, 0, 0}, 0, 0}),
        "Fully blocked sound is not perceived");
  Check(!sensor.Hear({perception::SoundKind::Gunshot, {1, 0, 0}, 3, 1},
                     {{0, 0, 0}, 1, 1}),
        "Full environmental masking hides the event");
  Check(!sensor.Hear({perception::SoundKind::Breathing, {100, 0, 0}, 4, 1},
                     {{0, 0, 0}, 1, 0}),
        "Distant breathing is inaudible");
}

void UncertaintyAndReproducibility() {
  perception::PerceptionCore first({}, 123), second({}, 123);
  perception::SoundEvent event{perception::SoundKind::Explosion,
                                {30, 10, 0}, 5, 1};
  auto clear = first.Hear(event, {{0, 0, 0}, 1, 0});
  auto occluded = second.Hear(event, {{0, 0, 0}, 0.6, 0.2});
  Check(clear && occluded, "Both explosions audible");
  Check(occluded->estimate->radiusMeters > clear->estimate->radiusMeters,
        "Occlusion and masking widen uncertainty");
  Check(occluded->strength < clear->strength,
        "Occlusion and masking weaken evidence");
  perception::PerceptionCore replay({}, 123);
  auto same = replay.Hear(event, {{0, 0, 0}, 1, 0});
  Check(same->estimate->center.x == clear->estimate->center.x &&
            same->estimate->center.y == clear->estimate->center.y,
        "Same seed and events produce repeatable estimates");
}

void EvidenceRequiresActualInspection() {
  perception::EvidenceMemory evidence;
  hunter::HunterCore hunter(hunter::Difficulty::Hard);
  Check(!evidence.Inspect("east-door", "closed", {8, 1, 0}, 0),
        "First sighting establishes a baseline");
  Check(!evidence.Inspect("east-door", "closed", {8, 1, 0}, 1),
        "Unchanged object gives no clue");
  // World change at t=2 is deliberately not sent to the Hunter.
  Check(hunter.CurrentState() == hunter::State::Patrol,
        "Remote change is not magical knowledge");
  auto clue = evidence.Inspect("east-door", "open", {8, 1, 0}, 3, 0.7);
  Check(clue && clue->subject == hunter::Subject::Unknown,
        "Only a later inspection yields an anonymous clue");
  Check(clue->perceivedType == "east-door:open",
        "Evidence retains object and visible state");
  Check(hunter.Observe(*clue), "Hunter accepts clue");
  Check(hunter.CurrentState() == hunter::State::Investigating,
        "Observed changed door can prompt investigation");
  Check(!evidence.Inspect("east-door", "open", {8, 1, 0}, 4),
        "Repeated same observation does not spam stimuli");
  Check(!evidence.Inspect("light-2", "off", {9, 1, 0}, 5),
        "Other object has independent baseline");
  Check(evidence.Inspect("east-door", "closed", {8, 1, 0}, 6).has_value(),
        "Changed-back state only noticed on reinspection");
  Check(hunter.RouteEvidence("east-door") == 0,
        "Environmental change is not a witnessed player route");
}

void InvalidInput() {
  perception::PerceptionCore sensor;
  bool threw = false;
  try {
    sensor.Hear({perception::SoundKind::Door, {0, 0, 0}, 0, 1},
                {{0, 0, 0}, 1.2, 0});
  } catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Invalid transmission rejected");
  perception::EvidenceMemory memory;
  memory.Inspect("camera", "on", {0, 0, 0}, 3);
  threw = false;
  try { memory.Inspect("camera", "off", {0, 0, 0}, 2); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Out-of-order inspection rejected");
}

} // namespace

int main() {
  try {
    ApproximateSoundAndHunter();
    DecoyAndMasking();
    UncertaintyAndReproducibility();
    EvidenceRequiresActualInspection();
    InvalidInput();
    std::cout << "PerceptionCore: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "PerceptionCore test failed after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
