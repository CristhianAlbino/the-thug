#include "StorySave.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace thug;

namespace {

int checks = 0;
void Check(bool okay, const char* reason) {
  ++checks;
  if (!okay) throw std::runtime_error(reason);
}

companion::Traits Neutral() { return {50, 50, 50, 50, 50, 50, 0, false}; }

struct Fixture {
  survival::Inventory joe{6, {{"bandage", survival::ItemCategory::Medical, 1, 3}}};
  companion::CompanionCore harley{companion::Character::Harley,
                                   companion::Difficulty::Hard, Neutral(), 11};
  companion::CompanionCore barth{companion::Character::Bartholomeu,
                                  companion::Difficulty::Hard, Neutral(), 22};
  companion::CompanionCore jack{companion::Character::Jack,
                                 companion::Difficulty::Hard, Neutral(), 33};
  hunter::HunterCore hunter{hunter::Difficulty::Hard, 44};
  rules::RunRules routes{rules::Difficulty::Hard};
  survival::SurvivalCore survival;
  perception::PerceptionCore hearing{{}, 55};
  perception::EvidenceMemory evidence;
  save::WorldState world{"hospital_test", {
      {"door-a", "door", {"closed", {1, 2, 0}, {}, true}}}};
  save::NarrativeState narrative{rules::Difficulty::Hard};

  save::StoryCampaign Members() {
    return {{{joe, harley, barth, jack}, hunter, routes, survival,
             hearing, evidence, world}, narrative};
  }
};

std::string Encode(save::StoryCampaign campaign) {
  std::ostringstream output;
  Check(save::SaveStoryCampaign(output, campaign) == save::Result::Ok,
        "Full story save succeeds");
  return output.str();
}

save::Result Decode(const std::string& data, save::StoryCampaign campaign) {
  std::istringstream input(data);
  return save::LoadStoryCampaign(input, campaign);
}

void StoryRoundTrip() {
  Fixture original;
  Check(original.joe.Add("bandage", 1) &&
        original.world.SetObject("door-a", {"open", {1, 2, 0}, {}, true}, 1),
        "Physical state changes");
  Check(original.narrative.RecordScene("N01", 1) &&
        original.narrative.RecordBanter("BAN01", 2) &&
        original.narrative.InspectDocument("DOC01", 3) &&
        original.narrative.InspectAudio("AUD01", 4),
        "Scenes, conversations and inspected journal entries recorded");
  auto state = original.narrative.CaptureSnapshot();
  state.act = 3;
  state.activeBanter = "BAN02";
  state.activeBanterLine = 2;
  state.activeScene = "N12";
  state.activeSceneLine = 1;
  state.durableFacts.insert("joe_found_record");
  state.outcomes["harley"].lastContactNode = "F3_ICU";
  state.keypadAttempts = 1;
  Check(original.narrative.RestoreSnapshot(state),
        "In-progress dialogue and durable evidence are valid");
  Check(original.world.AdvanceTo(10), "World clock includes narrative events");
  const auto saved = Encode(original.Members());
  Check(saved.rfind("THE_THUG_STORY_CAMPAIGN 1\n", 0) == 0,
        "Story saves have their own version header");

  Fixture resumed;
  Check(Decode(saved, resumed.Members()) == save::Result::Ok,
        "Narrative and core load as one unit");
  Check(Encode(resumed.Members()) == saved,
        "Narrative, RNG and physical state round trip byte for byte");
  const auto restored = resumed.narrative.CaptureSnapshot();
  Check(restored.seenScenes.count("N01") &&
        restored.deliveredBanter.count("BAN01") &&
        restored.inspectedDocuments.count("DOC01") &&
        restored.inspectedAudio.count("AUD01") &&
        restored.activeBanter == "BAN02" &&
        restored.activeBanterLine == 2 &&
        restored.lastBanterCompletedAt == 2 &&
        restored.activeSceneLine == 1 &&
        restored.keypadAttempts == 1 &&
        restored.outcomes.at("harley").lastContactNode == "F3_ICU",
        "All narrative progress including interrupted lines resumes");
  Check(!resumed.narrative.RecordScene("N01", 11) &&
        !resumed.narrative.RecordBanter("BAN01", 11) &&
        !resumed.narrative.InspectDocument("DOC01", 11),
        "Completed dialogue and inspected document cannot replay");
  Check(resumed.world.Find("door-a")->stateId == "open" &&
        resumed.joe.Quantity("bandage") == 1,
        "World and inventory match the saved narrative");

  const auto Reject = [&](const std::string& bytes, save::Result result) {
    Check(Decode(bytes, resumed.Members()) == result,
          "Rejected story file has the expected error");
    Check(Encode(resumed.Members()) == saved,
          "Rejected file changes neither story nor any core state");
  };
  Reject(saved.substr(0, saved.size() - 10), save::Result::Malformed);
  auto tampered = saved;
  tampered[tampered.find("DOC01")] = 'Z';
  Reject(tampered, save::Result::Corrupt);
  auto future = saved;
  future.replace(0, std::string("THE_THUG_STORY_CAMPAIGN 1").size(),
                 "THE_THUG_STORY_CAMPAIGN 2");
  Reject(future, save::Result::UnsupportedVersion);
  Reject(saved + "EXTRA", save::Result::Malformed);

  std::ostringstream coreOnly;
  Check(save::SaveCampaign(coreOnly, resumed.Members().core) == save::Result::Ok,
        "Old core-only format remains readable by its own loader");
  Reject(coreOnly.str(), save::Result::Malformed);

  auto invalid = resumed.narrative.CaptureSnapshot();
  invalid.outcomes["harley"].dead = true;
  invalid.outcomes["harley"].captured = true;
  Check(!resumed.narrative.RestoreSnapshot(invalid),
        "Contradictory companion outcomes cannot enter story state");
  invalid = resumed.narrative.CaptureSnapshot();
  invalid.seenScenes.insert("N34");
  invalid.difficulty = rules::Difficulty::Easy;
  Check(!resumed.narrative.RestoreSnapshot(invalid),
        "Different difficulty cannot import hidden narrative flags");
  save::NarrativeState normal{rules::Difficulty::Normal};
  auto normalState = normal.CaptureSnapshot();
  normalState.seenScenes.insert("N34");
  Check(!normal.RestoreSnapshot(normalState),
        "Ordinary difficulty cannot learn the hidden Thomas scene");
  normalState = normal.CaptureSnapshot();
  normalState.witnessedClues.insert("J1");
  Check(!normal.RestoreSnapshot(normalState),
        "Ordinary difficulty cannot learn Punitive-only clues");

  invalid = resumed.narrative.CaptureSnapshot();
  invalid.joeEscaped = true;
  Check(resumed.narrative.RestoreSnapshot(invalid),
        "Narrative can receive a verified exit from the adapter");
  std::ostringstream impossible;
  Check(save::SaveStoryCampaign(impossible, resumed.Members()) ==
            save::Result::IncompatibleRoster && impossible.str().empty(),
        "A narrated escape without a RunRules ending cannot be saved");
  invalid.joeEscaped = false;
  Check(resumed.narrative.RestoreSnapshot(invalid),
        "Removing the unverified exit returns to the previous story");

  Fixture clock;
  Check(clock.narrative.RecordScene("N01", 1), "Narrative clock advances");
  std::ostringstream blank;
  Check(save::SaveStoryCampaign(blank, clock.Members()) ==
            save::Result::IncompatibleRoster && blank.str().empty(),
        "Save refuses story events ahead of the world clock");
}

void DifficultyAndProgressBoundaries() {
  for (auto difficulty : {rules::Difficulty::Easy, rules::Difficulty::Normal,
                         rules::Difficulty::Hard, rules::Difficulty::Punitive}) {
    save::NarrativeState state(difficulty);
    const bool confirmed = difficulty == rules::Difficulty::Hard ||
                           difficulty == rules::Difficulty::Punitive;
    auto pending = state.CaptureSnapshot();
    pending.activeScene = "N34";
    Check(state.RestoreSnapshot(pending) == confirmed,
          "Unfinished scene obeys the same Thomas gate as completed scenes");
    Check(state.InspectDocument("DOC16", 1) == confirmed &&
          state.InspectAudio("AUD08", 2) == confirmed,
          "Restricted journal entries cannot leak through save state");
    Check(state.InspectAudio("AUD07", 3) == (difficulty == rules::Difficulty::Punitive),
          "Jack recording is Punitive-only");
  }
  save::NarrativeState punitive(rules::Difficulty::Punitive);
  Check(!punitive.WitnessClue("J1", 1), "Punitive clue cannot precede its narrative act");
  auto next = punitive.CaptureSnapshot();
  next.act = 4;
  Check(punitive.RestoreSnapshot(next) && punitive.WitnessClue("J2", 1) &&
        punitive.WitnessClue("J3", 2) && !punitive.WitnessClue("J4", 3),
        "Missing J1 is allowed, premature J4 is not");
  next = punitive.CaptureSnapshot();
  next.act = 5;
  Check(punitive.RestoreSnapshot(next) && punitive.WitnessClue("J4", 3),
        "Late reveal accepts J2 plus J3 without forcing the missed J1");
  next = punitive.CaptureSnapshot();
  next.durableFacts.insert("jack_final_encounter");
  Check(!punitive.RestoreSnapshot(next) &&
        punitive.CaptureSnapshot().durableFacts.empty(),
        "Transient encounter cannot survive as a durable fact");
  next.durableFacts = {"J1"};
  Check(!punitive.RestoreSnapshot(next), "Clue gates cannot be bypassed through generic facts");
  save::NarrativeState normal(rules::Difficulty::Normal);
  next = normal.CaptureSnapshot();
  next.durableFacts.insert("joe_directly_witnessed_thomas_death");
  Check(!normal.RestoreSnapshot(next), "Generic facts cannot smuggle Thomas proof into Normal");

  Fixture escaped;
  Check(!escaped.narrative.RecordScene("N30:exit", 1),
        "Coda cannot be recorded before Joe escaped");
  Check(escaped.routes.Apply(rules::Event::TunnelFound, 1) &&
        escaped.routes.Apply(rules::Event::TunnelExitReached, 2) &&
        escaped.world.AdvanceTo(2), "Confirmed tunnel ending is recorded by RunRules");
  next = escaped.narrative.CaptureSnapshot();
  next.joeEscaped = true;
  Check(escaped.narrative.RestoreSnapshot(next) &&
        escaped.narrative.RecordScene("N30:exit", 2), "Confirmed route can receive its coda");
  const auto bytes = Encode(escaped.Members());
  Fixture loaded;
  Check(Decode(bytes, loaded.Members()) == save::Result::Ok &&
        loaded.narrative.CaptureSnapshot().seenScenes.count("N30:exit"),
        "Coda stage survives complete story round trip");
  next.seenScenes = {"N28:exit"};
  Check(escaped.narrative.RestoreSnapshot(next), "Ledger can receive an adapter claim");
  std::ostringstream rejected;
  Check(save::SaveStoryCampaign(rejected, escaped.Members()) ==
        save::Result::IncompatibleRoster && rejected.str().empty(),
        "Full checkpoint rejects coda for the wrong RunRules route");
}

} // namespace

int main() {
  try {
    StoryRoundTrip();
    DifficultyAndProgressBoundaries();
    std::cout << "StorySave: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "StorySave failed after " << checks <<
        " checks: " << error.what() << '\n';
    return 1;
  }
}
