#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "CampaignIO.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace thug;
using campaign::ActionKind;
using campaign::Status;

namespace {
int checks = 0;
void Check(bool value, const char* reason) {
  ++checks;
  if (!value) throw std::runtime_error(reason);
}

std::string Save(const campaign::CampaignCore& core) {
  std::ostringstream out;
  Check(core.Save(out) == save::Result::Ok, "Coordinated runtime checkpoint succeeds");
  return out.str();
}

save::Result Load(campaign::CampaignCore& core, const std::string& bytes) {
  std::istringstream in(bytes);
  return core.Load(in);
}

campaign::Action Action(const campaign::CampaignCore& core, std::string event,
                        ActionKind kind, std::string target, std::string node, int argument = 0) {
  return {std::move(event), kind, std::move(target), std::move(node), core.Now(), argument, true};
}

int Quantity(const campaign::CampaignCore& core, const char* item) {
  const auto inventory = core.Inventory();
  const auto it = inventory.contents.find(item);
  return it == inventory.contents.end() ? 0 : it->second;
}

void Apply(campaign::CampaignCore& core, const char* event, ActionKind kind,
           const char* target, const char* node, int argument = 0) {
  Check(core.Apply(Action(core, event, kind, target, node, argument)).status == Status::Applied,
        "Authored interaction completes through coordinator");
}

void RejectedUnchanged(campaign::CampaignCore& core, const campaign::Action& action, Status status) {
  const auto before = Save(core);
  Check(core.Apply(action).status == status, "Invalid transaction has the expected rejection");
  Check(Save(core) == before, "Rejected transaction leaves every subsystem and ledger unchanged");
}

void InitialDataAndPause() {
  for (const auto difficulty : {rules::Difficulty::Easy, rules::Difficulty::Normal,
                                rules::Difficulty::Hard, rules::Difficulty::Punitive}) {
    campaign::CampaignCore core(difficulty, 42);
    const auto profile = campaign::data::For(difficulty);
    Check(core.Inventory().capacitySlots == profile.capacity &&
          core.Inventory().contents == profile.loadout,
          "Native campaign loads authored balance and actual loadout");
    Check(core.Companion(companion::Character::Harley).traits.trust == profile.companions[0].trust,
          "Companions use authored values instead of fixture traits");
    Check(core.World().objects.size() >= 40 && core.Ending() == rules::Ending::None,
          "Logical hospital devices initialize with unfinished campaign");
    Apply(core, "injury", ActionKind::Injury, "joe", "F4_START", 3);
    Check(core.Tick(4, survival::Activity::Sprint).status == Status::Applied,
          "Shared simulation advances survival");
    Check(core.Now() == 4 && core.Hunter().nowSeconds == 4 &&
          core.Companion(companion::Character::Jack).nowSeconds == 4 && core.Narrative().nowSeconds == 4,
          "Continuous simulation clocks stay synchronized");
    core.SetPaused(true);
    const auto paused = Save(core);
    Check(core.Tick(60, survival::Activity::Rest).status == Status::Paused && Save(core) == paused,
          "Pause freezes bleeding, stamina, clocks and all state");
    RejectedUnchanged(core, Action(core, "paused-treatment", ActionKind::Treat, "treatment", "F1_PHARMACY"), Status::Paused);
    campaign::CampaignCore resumed(difficulty, 999);
    Check(Load(resumed, paused) == save::Result::Ok && resumed.Paused() && Save(resumed) == paused,
          "Checkpoint resumes paused with exact states and saved RNG, despite new constructor seed");
    resumed.SetPaused(false);
    Check(resumed.Tick(1, survival::Activity::Rest).advancedSeconds == 1 && resumed.Now() == 5,
          "Resuming advances only gameplay time");
  }
}

void TransactionalResources() {
  campaign::CampaignCore core(rules::Difficulty::Hard);
  auto unverified = Action(core, "unverified", ActionKind::Collect, "P_TREATMENT", "F1_PHARMACY");
  unverified.physicallyVerified = false;
  RejectedUnchanged(core, unverified, Status::InvalidAction);
  RejectedUnchanged(core, Action(core, "wrong-room", ActionKind::Collect, "P_TREATMENT", "F4_START"), Status::MissingRequirement);
  Apply(core, "take-med", ActionKind::Collect, "P_TREATMENT", "F1_PHARMACY");
  Check(Quantity(core, "treatment") == 1 && !core.World().objects.at("P_TREATMENT").present,
        "Collection conserves the item between world and bag");
  RejectedUnchanged(core, Action(core, "unneeded-med", ActionKind::Treat, "treatment", "F1_PHARMACY"), Status::MissingRequirement);
  Apply(core, "hit", ActionKind::Injury, "joe", "F1_PHARMACY", 3);
  Apply(core, "treat", ActionKind::Treat, "treatment", "F1_PHARMACY");
  Check(Quantity(core, "treatment") == 0 && core.Survival().injury == survival::Injury::Moderate && !core.Survival().bleeding,
        "Treatment consumes once and updates injury/bleeding in the same commit");
  const auto treated = Save(core);
  auto replay = Action(core, "treat", ActionKind::Treat, "treatment", "F1_PHARMACY");
  Check(core.Apply(replay).status == Status::AlreadyApplied && Save(core) == treated,
        "Repeat of confirmed action neither consumes nor heals again");
  replay.target = "battery";
  Check(core.Apply(replay).status == Status::InvalidAction, "Wrong item category is rejected");
  replay = Action(core, "treat", ActionKind::Treat, "treatment", "F4_START");
  RejectedUnchanged(core, replay, Status::Conflict);
  RejectedUnchanged(core, Action(core, "new-collect", ActionKind::Collect, "P_TREATMENT", "F1_PHARMACY"), Status::MissingRequirement);

  campaign::CampaignCore loaded(rules::Difficulty::Hard);
  Check(Load(loaded, treated) == save::Result::Ok, "Load restores collection and use receipts");
  Check(loaded.Apply(Action(loaded, "treat", ActionKind::Treat, "treatment", "F1_PHARMACY")).status == Status::AlreadyApplied,
        "Replay after checkpoint cannot repeat consumption");
  Apply(loaded, "ammo", ActionKind::Collect, "P_MAGAZINE", "F2_SUPPLY");
  const int ammo = Quantity(loaded, "magazine");
  Apply(loaded, "drop-ammo", ActionKind::Drop, "P_MAGAZINE", "F2_SUPPLY");
  Check(Quantity(loaded, "magazine") == ammo - 1 && loaded.World().objects.at("P_MAGAZINE").present,
        "Dropping moves stock back to an authored world slot");
  Apply(loaded, "recollect", ActionKind::Collect, "P_MAGAZINE", "F2_SUPPLY");
  Check(Quantity(loaded, "magazine") == ammo, "Recollection restores, never duplicates stock");
  const auto loyalty = loaded.Companion(companion::Character::Bartholomeu).traits.loyalty;
  Apply(loaded, "give-barth", ActionKind::ShareAmmo, "bartholomeu", "F2_SUPPLY");
  Check(Quantity(loaded, "magazine") == ammo - 1 &&
        loaded.Companion(companion::Character::Bartholomeu).traits.ammunition == 1 &&
        loaded.Companion(companion::Character::Bartholomeu).traits.loyalty == loyalty + 10,
        "Ammo share commits bag, companion resource and relationship together");

  campaign::CampaignCore stabilized(rules::Difficulty::Easy);
  Apply(stabilized, "injury", ActionKind::Injury, "joe", "F1_PHARMACY", 3);
  stabilized.Tick(2, survival::Activity::Rest);
  Apply(stabilized, "stabilize", ActionKind::Stabilize, "treatment", "F1_PHARMACY");
  const auto blood = stabilized.Survival().bloodLoss;
  Check(Quantity(stabilized, "treatment") == 0 && !stabilized.Survival().bleeding &&
        stabilized.Survival().injury == survival::Injury::Severe,
        "Stabilization consumes once and stops bleeding without healing severity");
  stabilized.Tick(10, survival::Activity::Rest);
  Check(stabilized.Survival().bloodLoss == blood, "Stabilized wound does not continue bleeding");
  RejectedUnchanged(stabilized, Action(stabilized, "no-stock", ActionKind::Treat, "treatment", "F1_PHARMACY"), Status::MissingRequirement);
}

void FullBagAndPrerequisiteRollback() {
  campaign::CampaignCore core(rules::Difficulty::Hard);
  Apply(core, "med", ActionKind::Collect, "P_TREATMENT", "F1_PHARMACY");
  Apply(core, "radio", ActionKind::Collect, "P_RADIO", "F1_RADIO");
  Apply(core, "battery", ActionKind::Collect, "P_BATTERY", "F2_SERVER");
  RejectedUnchanged(core, Action(core, "full-tools", ActionKind::Collect, "P_WINDOW_TOOL", "F2_SUPPLY"), Status::MissingRequirement);
  Check(core.World().objects.at("P_WINDOW_TOOL").present, "Full bag cannot delete a needed tool");

  campaign::CampaignCore noRadio(rules::Difficulty::Hard);
  Apply(noRadio, "battery", ActionKind::Collect, "P_BATTERY", "F2_SERVER");
  RejectedUnchanged(noRadio, Action(noRadio, "install-too-soon", ActionKind::Use, "install_battery", "F1_RADIO"), Status::MissingRequirement);
  Check(Quantity(noRadio, "battery") == 1 && noRadio.World().objects.at("RADIO_INSTALL").stateId == "empty",
        "Failed prerequisite rolls back tentative resource removal and device change");
  RejectedUnchanged(noRadio, Action(noRadio, "key-too-soon", ActionKind::Collect, "P_GARAGE_KEY", "F2_SAFE"), Status::MissingRequirement);
  RejectedUnchanged(noRadio, Action(noRadio, "unlock-no-key", ActionKind::UnlockDoor, "unlock_exams", "F3_EXAMS"), Status::MissingRequirement);
  Apply(noRadio, "service-key", ActionKind::Collect, "P_SERVICE_KEY", "B1_STORAGE");
  Apply(noRadio, "unlock", ActionKind::UnlockDoor, "unlock_exams", "F3_EXAMS");
  Check(Quantity(noRadio, "service_key") == 1 && noRadio.World().objects.at("D_F3_EXAMS").stateId == "closed",
        "Reusable authorization unlocks real door without consuming key");
}

void DoorSoundAndHiddenState() {
  campaign::CampaignCore core(rules::Difficulty::Hard, 17);
  const auto action = Action(core, "open", ActionKind::OpenDoor, "D_F2_SERVER", "F2_CENTRAL");
  const auto opened = core.Apply(action);
  Check(opened.status == Status::Applied && opened.signal, "Opening produces a physical sound output");
  Check(core.Hunter().state == hunter::State::Patrol, "World action does not magically inform Hunter");
  Check(core.Hear({opened.signal->sound, opened.signal->positionMeters, core.Now(), opened.signal->energy}, {{5, 0, 0}, 1, 0}) &&
        core.Hunter().state == hunter::State::Investigating, "Only delivered audible perception reaches Hunter");
  Check(!core.Apply(action).signal && core.Apply(action).status == Status::AlreadyApplied,
        "Action replay does not emit duplicate door audio");
  Apply(core, "blackout", ActionKind::Circuit, "C_SECURITY", "F2_SERVER", 0);
  Check(!core.Camera("CAM_LOBBY") && core.World().objects.at("D_F2_SERVER").stateId == "open",
        "Blackout changes feed while preserving actual open door");
  Apply(core, "close", ActionKind::CloseDoor, "D_F2_SERVER", "F2_CENTRAL");
  RejectedUnchanged(core, Action(core, "open-no-power", ActionKind::OpenDoor, "D_F2_SERVER", "F2_CENTRAL"), Status::MissingRequirement);
  Apply(core, "hide", ActionKind::EnterHide, "H_F3_ISOLATION", "F3_ISOLATION");
  const auto hidden = Save(core);
  campaign::CampaignCore resumed(rules::Difficulty::Hard);
  Check(Load(resumed, hidden) == save::Result::Ok && resumed.HidingPlace() == "H_F3_ISOLATION",
        "Joe hiding identity and occupied world actor restore together");
  Check(resumed.Tick(1, survival::Activity::Sprint).status == Status::InvalidAction,
        "Cannot simultaneously remain in hiding and sprint");
  RejectedUnchanged(resumed, Action(resumed, "remote-item", ActionKind::Collect, "P_BATTERY", "F2_SERVER"), Status::MissingRequirement);
  RejectedUnchanged(resumed, Action(resumed, "remote-injury", ActionKind::Injury, "joe", "F2_SERVER", 1), Status::MissingRequirement);
  Apply(resumed, "leave", ActionKind::LeaveHide, "H_F3_ISOLATION", "F3_ISOLATION");
  Check(resumed.HidingPlace().empty(), "Leaving clears identity and occupancy");
}

void RoutesAndTerminalState() {
  campaign::CampaignCore window(rules::Difficulty::Hard);
  Apply(window, "tool", ActionKind::Collect, "P_WINDOW_TOOL", "F2_SUPPLY");
  RejectedUnchanged(window, Action(window, "no-discovery", ActionKind::Use, "unblock_window", "F3_WINDOW"), Status::MissingRequirement);
  Apply(window, "find", ActionKind::Trigger, "find_window", "F3_WINDOW");
  Apply(window, "unblock", ActionKind::Use, "unblock_window", "F3_WINDOW");
  Apply(window, "exit", ActionKind::Trigger, "window_exit", "X_WINDOW");
  Check(window.Ending() == rules::Ending::WindowEscape && window.Narrative().joeEscaped,
        "Confirmed exit synchronizes campaign result and narrative");
  const auto finished = Save(window);
  Check(window.Tick(100, survival::Activity::Sprint).status == Status::Finished && Save(window) == finished,
        "Terminal campaign does not advance gameplay");
  RejectedUnchanged(window, Action(window, "post-end-damage", ActionKind::Injury, "joe", "X_WINDOW", 3), Status::Finished);

  campaign::CampaignCore garage(rules::Difficulty::Hard);
  Apply(garage, "code-entered", ActionKind::SubmitKeypad, director::Content().garageCode.c_str(), "F2_SAFE");
  Apply(garage, "key", ActionKind::Collect, "P_GARAGE_KEY", "F2_SAFE");
  RejectedUnchanged(garage, Action(garage, "release-unpowered", ActionKind::Use, "release_garage", "G_CONTROL"), Status::MissingRequirement);
  Apply(garage, "power", ActionKind::Circuit, "C_SERVICE", "B1_GENERATOR", 1);
  Apply(garage, "release", ActionKind::Use, "release_garage", "G_CONTROL");
  Apply(garage, "exit", ActionKind::Trigger, "garage_exit", "X_GARAGE");
  Check(garage.Ending() == rules::Ending::GarageEscape && Quantity(garage, "garage_key") == 1,
        "Garage exit requires actual key, prepared world gate and energy");

  campaign::CampaignCore roof(rules::Difficulty::Hard);
  Apply(roof, "radio", ActionKind::Collect, "P_RADIO", "F1_RADIO");
  Apply(roof, "battery", ActionKind::Collect, "P_BATTERY", "F2_SERVER");
  Apply(roof, "install", ActionKind::Use, "install_battery", "F1_RADIO");
  RejectedUnchanged(roof, Action(roof, "contact-blocked", ActionKind::Use, "radio_contact", "F1_RADIO"), Status::MissingRequirement);
  Apply(roof, "interference", ActionKind::Use, "clear_interference", "F2_CAMERAS");
  Apply(roof, "contact", ActionKind::Use, "radio_contact", "F1_RADIO");
  Apply(roof, "confirm", ActionKind::Trigger, "helicopter_confirmed", "F1_RADIO");
  const auto mid = Save(roof);
  campaign::CampaignCore onTime(rules::Difficulty::Hard);
  Check(Load(onTime, mid) == save::Result::Ok, "Prepared route restores for timely physical exit");
  onTime.Tick(1, survival::Activity::Rest);
  Apply(onTime, "roof-escape", ActionKind::Trigger, "roof_exit", "X_ROOF");
  Check(onTime.Ending() == rules::Ending::RoofEscape && onTime.Narrative().joeEscaped,
        "Timely verified roof exit confirms conventional ending");
  Check(Load(onTime, Save(onTime)) == save::Result::Ok, "Successful roof ending restores coherently");
  campaign::CampaignCore continued(rules::Difficulty::Hard);
  Check(Load(continued, mid) == save::Result::Ok, "Prepared timed route survives checkpoint");
  Check(continued.Tick(301, survival::Activity::Rest).status == Status::Applied, "Coordinator advances route deadline");
  RejectedUnchanged(continued, Action(continued, "late-roof", ActionKind::Trigger, "roof_exit", "X_ROOF"), Status::MissingRequirement);
  Apply(continued, "other-route", ActionKind::Trigger, "find_tunnel", "B1_TUNNEL");
  Apply(continued, "escape", ActionKind::Trigger, "tunnel_exit", "X_TUNNEL");
  Check(continued.Ending() == rules::Ending::TunnelEscape, "Failed timed route still permits a verified alternative");
}

void DeathAndDawn() {
  auto tuning = survival::Tuning{};
  tuning.bleedingPerSecond = 10;
  tuning.collapseBloodLoss = 50;
  campaign::CampaignCore dying(rules::Difficulty::Hard, campaign::data::Hospital(), 1, tuning);
  Apply(dying, "fatal-hit", ActionKind::Injury, "joe", "F3_CENTRAL", 3);
  const auto advanced = dying.Tick(100, survival::Activity::Rest);
  Check(advanced.advancedSeconds == 10 && advanced.survival.becameDead && dying.Now() == 10,
        "Long step stops at exact fatal event, not requested frame end");
  Check(dying.Ending() == rules::Ending::Died && dying.Survival().dead && !dying.Narrative().joeEscaped,
        "Survival death and campaign ending commit together");
  campaign::CampaignCore resumed(rules::Difficulty::Hard, campaign::data::Hospital(), 99, tuning);
  Check(Load(resumed, Save(dying)) == save::Result::Ok && resumed.Ending() == rules::Ending::Died,
        "Fatal boundary produces a coherent loadable checkpoint");

  campaign::CampaignCore easy(rules::Difficulty::Easy);
  Check(easy.Tick(1799, survival::Activity::Rest).status == Status::Applied, "Time can progress toward authored dawn");
  RejectedUnchanged(easy, Action(easy, "premature-dawn", ActionKind::Trigger, "dawn_confirmed", "F1_LOBBY"), Status::MissingRequirement);
  easy.Tick(1, survival::Activity::Rest);
  Check(easy.Ending() == rules::Ending::None, "Clock alone never confirms physical dawn");
  Apply(easy, "dawn", ActionKind::Trigger, "dawn_confirmed", "F4_START");
  Check(easy.Ending() == rules::Ending::SurvivedDawn, "Permitted dawn requires clock and verified world event");
  campaign::CampaignCore punitive(rules::Difficulty::Punitive);
  punitive.Tick(20000, survival::Activity::Rest);
  RejectedUnchanged(punitive, Action(punitive, "dawn", ActionKind::Trigger, "dawn_confirmed", "F4_START"), Status::MissingRequirement);
  Check(punitive.Ending() == rules::Ending::None, "Punitive has no automatic dawn victory");
}

void CheckpointFailureAndIdentity() {
  campaign::CampaignCore core(rules::Difficulty::Hard);
  Apply(core, "pickup", ActionKind::Collect, "P_BATTERY", "F2_SERVER");
  const auto clean = Save(core);
  const auto Reject = [&](std::string bytes, save::Result expected) {
    Check(Load(core, bytes) == expected, "Bad runtime file reports correct failure");
    Check(Save(core) == clean, "Rejected runtime file preserves state and receipts");
  };
  Reject(clean.substr(0, clean.size() - 5), save::Result::Malformed);
  auto changed = clean;
  changed[changed.find("RECEIPT \"pickup\"") + 10] = 'X';
  Reject(changed, save::Result::Corrupt);
  Reject(clean + "EXTRA", save::Result::Malformed);
  changed = clean;
  changed.replace(0, std::string("THE_THUG_RUNTIME_CAMPAIGN 7").size(), "THE_THUG_RUNTIME_CAMPAIGN 8");
  Reject(changed, save::Result::UnsupportedVersion);
  changed = clean;
  const auto offset = changed.find("CONTENT ") + 8;
  changed[offset] = changed[offset] == '0' ? '1' : '0';
  const auto checksum = changed.rfind("CHECKSUM ");
  changed = changed.substr(0, checksum) + "CHECKSUM " + campaign::detail::Hash(changed.substr(0, checksum)) + '\n';
  Reject(changed, save::Result::IncompatibleRoster);
  changed = clean;
  const auto receipt = changed.find("RECEIPT \"pickup\"");
  const auto wrongNode = changed.find("F2_SERVER", receipt);
  changed.replace(wrongNode, 9, "F3_CENTRAL");
  const auto receiptChecksum = changed.rfind("CHECKSUM ");
  changed = changed.substr(0, receiptChecksum) + "CHECKSUM " + campaign::detail::Hash(changed.substr(0, receiptChecksum)) + '\n';
  Reject(changed, save::Result::IncompatibleRoster);
  Reject(std::string(campaign::detail::MaxRuntimeBytes + 1, 'x'), save::Result::TooLarge);
  std::ostringstream failedOutput;
  failedOutput.setstate(std::ios::badbit);
  Check(core.Save(failedOutput) == save::Result::IoError && Save(core) == clean,
        "Failed checkpoint output preserves live campaign state");
  campaign::CampaignCore different(rules::Difficulty::Normal);
  Check(Load(different, clean) == save::Result::IncompatibleRoster,
        "Different difficulty cannot import runtime progress");
  auto otherDefinition = campaign::data::Hospital();
  otherDefinition.uses.front().after = "opened_differently";
  campaign::CampaignCore altered(rules::Difficulty::Hard, otherDefinition);
  Check(Load(altered, clean) == save::Result::IncompatibleRoster,
        "Changed authored interaction semantics invalidate old runtime save");
  auto invalidDefinition = campaign::data::Hospital();
  for (auto& object : invalidDefinition.objects)
    if (object.id == invalidDefinition.uses.front().object) object.initial.stateId = "incompatible_initial";
  bool rejected = false;
  try { campaign::CampaignCore invalid(rules::Difficulty::Hard, invalidDefinition); }
  catch (const std::invalid_argument&) { rejected = true; }
  Check(rejected, "Invalid initial interaction state rejects authoring before campaign starts");
}
} // namespace

int main() {
  try {
    InitialDataAndPause();
    TransactionalResources();
    FullBagAndPrerequisiteRollback();
    DoorSoundAndHiddenState();
    RoutesAndTerminalState();
    DeathAndDawn();
    CheckpointFailureAndIdentity();
    std::cout << "CampaignCore: " << checks << " checks passed\n";
  } catch (const std::exception& e) {
    std::cerr << "CampaignCore failed after " << checks << " checks: " << e.what() << '\n';
    return 1;
  }
}
