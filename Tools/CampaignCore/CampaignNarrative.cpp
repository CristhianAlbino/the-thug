#include "CampaignState.h"
#include "CompanionIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
bool ProtectedFact(const std::string& id) {
  static const std::set<std::string> owned{
    "joe_injured", "joe_bleeding", "harley_injured", "joe_abandoned_harley", "barth_betrayal_executed", "garage_escaped", "roof_escaped", "tunnel_escaped", "window_escaped",
    "garage_gate_released", "window_unblocked", "radio_contact_established", "helicopter_confirmed",
    "hide_victory", "dawn_victory", "rare_confrontation", "rare_destruction",
    "jack_previous_encounter_heard", "thomas_promised_coffee",
    "code_a", "code_b", "code_c", "key_known", "safe_known", "isolation_known"};
  return owned.count(id) || id.rfind("appearance:", 0) == 0 || id.rfind("route_failed:", 0) == 0 || id.rfind("doc", 0) == 0 ||
      id.rfind("joe_shared_ammo_with_", 0) == 0 || id.rfind("joe_shared_treatment_with_", 0) == 0;
}
bool TerminalScene(const std::string& id) {
  return id == "N33" || id == "N32" || id == "N28:exit" || id == "N29:exit" || id == "N30:exit" || id == "N31:exit";
}
}

bool CampaignCore::NarrativeStaticValid(const Action& a) const {
  if (!detail::Token(a.eventId) || !detail::Token(a.target) || !definition_->nodes.count(a.node) ||
      !std::isfinite(a.atSeconds) || a.atSeconds < 0 || a.kind < ActionKind::BeginScene || a.kind > ActionKind::AdvanceAct) return false;
  const auto block = director::Content().blocks.find(a.target);
  if (block != director::Content().blocks.end() && !(block->second.modes & (1u << static_cast<int>(state_->routes.DifficultyLevel())))) return false;
  if (a.kind == ActionKind::SceneCue || a.kind == ActionKind::BanterCue) {
    const auto cue = director::Content().cues.find(a.cueId);
    if (!a.presentationSerial || a.argument < 0 || cue == director::Content().cues.end() || cue->second.owner != a.target) return false;
  } else if (!a.cueId.empty()) return false;
  if (a.kind == ActionKind::CancelScene || a.kind == ActionKind::CancelBanter) {
    if (!a.presentationSerial || a.argument) return false;
  } else if (a.kind != ActionKind::SceneCue && a.kind != ActionKind::BanterCue && a.presentationSerial) return false;
  if (a.kind != ActionKind::AdvanceAct && a.kind != ActionKind::SceneCue && a.kind != ActionKind::BanterCue && a.argument) return false;
  switch (a.kind) {
    case ActionKind::BeginScene: case ActionKind::SceneCue: case ActionKind::CancelScene:
      return a.target == "N26" || a.target == "J4" || director::Content().scenes.count(a.target) ||
          (a.target.rfind("SP_", 0) == 0 && director::Content().blocks.count(a.target)) ||
          (a.target.rfind("N26:", 0) == 0 && director::Content().blocks.count(a.target)) || TerminalScene(a.target);
    case ActionKind::BeginBanter: case ActionKind::BanterCue: case ActionKind::CancelBanter:
      return director::Content().banter.count(a.target);
    case ActionKind::InspectDocument: return a.target.rfind("DOC", 0) == 0 && director::Content().blocks.count(a.target);
    case ActionKind::InspectAudio: return a.target.rfind("AUD", 0) == 0 && director::Content().blocks.count(a.target);
    case ActionKind::WitnessClue: return a.target == "J1" || a.target == "J2" || a.target == "J3";
    case ActionKind::RememberFact: return director::PersistentFact(a.target) && !ProtectedFact(a.target);
    case ActionKind::AdvanceAct: return a.target == "act" && a.argument >= 2 && a.argument <= 5;
    default: return false;
  }
}
bool CampaignCore::NarrativeCoherent(const State& s) const {
  const auto story = s.narrative.CaptureSnapshot();
  if ((story.act > 1 && !story.seenScenes.count("N03")) ||
      bool(story.seenScenes.count("J4")) != bool(story.witnessedClues.count("J4"))) return false;
  std::set<std::string> scenes, banter, clues, deliveredFacts, ammoFacts;
  unsigned attempts = 0;
  std::uint64_t activations = 0;
  std::set<int> sceneConfirmed, banterConfirmed;
  for (const auto& [id, signature] : s.receipts) {
    (void)id;
    if (signature.rfind("Q ", 0) == 0) continue;
    if (signature.rfind("R ", 0) == 0) continue;
    if (signature.rfind("I ", 0) == 0) continue;
    if (signature.rfind("S ", 0) == 0) continue;
    if (signature.rfind("H ", 0) == 0) continue;
    if (signature.rfind("W ", 0) == 0) continue; // Combat ledger is validated independently.
    if (signature.rfind("C ", 0) == 0) {
      CompanionCommand c;
      if (!detail::ParseCompanion(signature, c)) return false;
      if (c.kind == CompanionOp::Transfer && c.actor == "joe" && c.item == "magazine") ammoFacts.insert("joe_shared_ammo_with_" + c.target);
      continue;
    }
    Action a; if (!detail::ParseSignature(signature, a)) return false;
    if (a.presentationSerial > s.presentation.serial) return false;
    if (a.kind == ActionKind::BeginScene || a.kind == ActionKind::BeginBanter) ++activations;
    if (a.kind == ActionKind::SceneCue || a.kind == ActionKind::BanterCue) {
      const auto cue = director::Content().cues.find(a.cueId);
      if (cue == director::Content().cues.end()) return false;
      if (a.kind == ActionKind::SceneCue) scenes.insert(a.target); else banter.insert(a.target);
      if (!cue->second.deliveredFact.empty()) deliveredFacts.insert(cue->second.deliveredFact);
      const bool main = a.kind == ActionKind::SceneCue;
      const auto activeSerial = main ? s.presentation.sceneSerial : s.presentation.banterSerial;
      if (activeSerial && a.presentationSerial == activeSerial) {
        const auto& plan = main ? s.presentation.scenePlan : s.presentation.banterPlan;
        const auto& owner = main ? story.activeScene : story.activeBanter;
        const auto cursor = main ? story.activeSceneLine : story.activeBanterLine;
        if (a.target != owner || a.argument < 0 || a.argument >= cursor ||
            static_cast<std::size_t>(a.argument) >= plan.size() || plan[a.argument] != a.cueId) return false;
        (main ? sceneConfirmed : banterConfirmed).insert(a.argument);
      }
    }
    if (a.kind == ActionKind::WitnessClue) clues.insert(a.target);
    if (a.kind == ActionKind::ShareAmmo) ammoFacts.insert("joe_shared_ammo_with_" + a.target);
    if (a.kind == ActionKind::SubmitKeypad) ++attempts;
  }
  if (attempts != story.keypadAttempts || activations != s.presentation.serial ||
      sceneConfirmed.size() != static_cast<std::size_t>(story.activeSceneLine) ||
      banterConfirmed.size() != static_cast<std::size_t>(story.activeBanterLine)) return false;
  for (const auto& scene : story.seenScenes) if (!scenes.count(scene)) return false;
  for (const auto& conversation : story.deliveredBanter) if (!banter.count(conversation)) return false;
  for (const auto& clue : story.witnessedClues)
    if (clue != "J4" && !clues.count(clue)) return false;
  for (const auto& flag : {"jack_previous_encounter_heard", "thomas_promised_coffee"})
    if (bool(story.durableFacts.count(flag)) != bool(deliveredFacts.count(flag))) return false;
  for (const auto& name : {"harley", "bartholomeu", "jack"}) {
    const auto flag = std::string("joe_shared_ammo_with_") + name;
    if (bool(story.durableFacts.count(flag)) != bool(ammoFacts.count(flag))) return false;
  }
  for (const auto& [id, block] : director::Content().blocks) {
    (void)block;
    if (id.rfind("DOC", 0) == 0 &&
        bool(story.durableFacts.count("doc" + id.substr(3) + "_read")) != bool(story.inspectedDocuments.count(id))) return false;
  }
  const std::map<std::string, std::string> proof{{"code_a","DOC01"},{"code_b","DOC02"},{"code_c","DOC03"},{"key_known","DOC04"},{"isolation_known","DOC07"}};
  for (const auto& [flag, id] : proof)
    if (bool(story.durableFacts.count(flag)) != bool(story.inspectedDocuments.count(id))) return false;
  if (bool(story.durableFacts.count("safe_known")) !=
      bool(story.inspectedDocuments.count("DOC04") && story.inspectedDocuments.count("DOC05"))) return false;
  return true;
}
bool CampaignCore::ValidObservation(const director::Observation& o) const {
  const auto story = state_->narrative.CaptureSnapshot();
  if (!director::ValidObservation(o) || o.difficulty != story.difficulty || o.act != story.act ||
      o.nowSeconds != Now() || !definition_->nodes.count(o.joeNode) || o.facts.size() > 1000 || o.companions.size() > 4) return false;
  for (const auto& fact : o.facts)
    if (!detail::Token(fact) || fact == "J1" || fact == "J2" || fact == "J3" || fact == "J4") return false;
  for (const auto& [name, actor] : o.companions)
    if ((name != "harley" && name != "bartholomeu" && name != "jack" && name != "thomas") ||
        !definition_->nodes.count(actor.node)) return false;
  return true;
}
director::Observation CampaignCore::NarrativeContext(const director::Observation& input, const State& s) const {
  auto o = input;
  for (auto it = o.facts.begin(); it != o.facts.end();) {
    if (ProtectedFact(*it)) it = o.facts.erase(it); else ++it;
  }
  const auto run = s.routes.CaptureSnapshot();
  const auto* key = s.world.Find("P_GARAGE_KEY");
  if (!run.safeOpened || !key || !key->present) o.facts.erase("key_inside_safe_observed");
  if (s.survival.CaptureSnapshot().injury != survival::Injury::None) o.facts.insert("joe_injured");
  if (s.bodies.at("harley").survival.CurrentInjury() != survival::Injury::None) o.facts.insert("harley_injured");
  if (s.survival.CaptureSnapshot().bleeding) o.facts.insert("joe_bleeding");
  if (run.garageGate) o.facts.insert("garage_gate_released");
  if (run.windowUnblocked) o.facts.insert("window_unblocked");
  if (run.radioContact) o.facts.insert("radio_contact_established");
  if (run.helicopterDeadline && !run.roofFailed) o.facts.insert("helicopter_confirmed");
  if (run.garageFailed) o.facts.insert("route_failed:garage");
  if (run.roofFailed) o.facts.insert("route_failed:roof");
  if (run.tunnelFailed) o.facts.insert("route_failed:tunnel");
  if (run.windowFailed) o.facts.insert("route_failed:window");
  if (run.result == rules::Ending::GarageEscape) o.facts.insert("garage_escaped");
  if (run.result == rules::Ending::RoofEscape) o.facts.insert("roof_escaped");
  if (run.result == rules::Ending::TunnelEscape) o.facts.insert("tunnel_escaped");
  if (run.result == rules::Ending::WindowEscape) o.facts.insert("window_escaped");
  if (run.result == rules::Ending::HunterAbandoned) o.facts.insert("hide_victory");
  if (run.result == rules::Ending::SurvivedDawn) o.facts.insert("dawn_victory");
  const auto story = s.narrative.CaptureSnapshot();
  if (run.result == rules::Ending::CoordinatedEscape) o.facts.insert("rare_confrontation");
  if (run.result == rules::Ending::BitterDestruction) o.facts.insert("rare_destruction");
  for (const auto& id : s.special.appearances) o.facts.insert("appearance:" + id);
  for (auto& [name, actor] : o.companions) {
    if (RescueBusy(s,name) || WeaponBusy(s,name)) actor.active=false;
    const auto found = story.outcomes.find(name);
    if (found != story.outcomes.end() && (found->second.dead || found->second.captured || found->second.escapedAlone ||
        (run.result == rules::Ending::CoordinatedEscape && !found->second.together) ||
        run.result == rules::Ending::BitterDestruction)) actor.active = false;
  }
  return o;
}

bool CampaignCore::QuietBanter(const State& s, const std::string& id) const {
  const auto block = director::Content().banter.find(id);
  if (block != director::Content().banter.end()) for (const auto& name : block->second.actors) {
    const auto quiet = s.interactions.quietUntil.find(name);
    if (quiet != s.interactions.quietUntil.end() && quiet->second > s.world.NowSeconds()) return true;
  }
  return false;
}
ActionResult CampaignCore::ApplyNarrative(const Action& a, const director::Observation& input) {
  if (!NarrativeStaticValid(a)) return {Status::InvalidAction, {}};
  const auto signature = detail::Signature(a);
  const auto receipt = state_->receipts.find(a.eventId);
  if (receipt != state_->receipts.end()) return {receipt->second == signature ? Status::AlreadyApplied : Status::Conflict, {}};
  if (Paused()) return {Status::Paused, {}};
  if (state_->special.phase == SpecialPhase::Collapsing ||
      (state_->special.phase == SpecialPhase::Confronting && Now() >= state_->special.boundaryAt)) return {Status::AwaitingPhysics, {}};
  if (a.atSeconds != Now() || !ValidObservation(input) || a.node != input.joeNode) return {Status::InvalidTime, {}};
  if (!a.physicallyVerified) return {Status::InvalidAction, {}};
  if (!RescueLocation(*state_,"joe",a.node) || (RescueBusy(*state_,"joe") &&
      a.kind!=ActionKind::CancelScene && a.kind!=ActionKind::CancelBanter)) return {Status::MissingRequirement,{}};
  if (Ending() != rules::Ending::None && a.kind != ActionKind::CancelScene && a.kind != ActionKind::CancelBanter &&
      !((a.kind == ActionKind::BeginScene || a.kind == ActionKind::SceneCue) && TerminalScene(a.target))) return {Status::Finished, {}};
  if (!state_->hidingPlace.empty() && a.node != std::find_if(definition_->hides.begin(), definition_->hides.end(),
      [&](const auto& hide) { return hide.id == state_->hidingPlace; })->atNode) return {Status::MissingRequirement, {}};
  if (state_->receipts.size() + detail::CombatReservedReceipts(state_->combat) + detail::SpecialReservedReceipts(state_->special) + detail::RescueReservedReceipts(state_->rescue) + detail::StrategyReservedReceipts(state_->strategy) >= detail::MaxReceipts) return {Status::LogFull, {}};
  auto next = std::make_unique<State>(*state_);
  auto story = next->narrative.CaptureSnapshot();
  const auto context = NarrativeContext(input, *next);
  bool accepted = false;
  switch (a.kind) {
    case ActionKind::BeginScene:
      accepted = director::BeginScene(a.target, context, story, next->presentation); break;
    case ActionKind::BeginBanter: {
      accepted = !QuietBanter(*next, a.target) && director::BeginBanter(a.target, context, story, next->presentation); break;
    }
    case ActionKind::SceneCue: case ActionKind::BanterCue: {
      const auto current = director::Current(context, story, next->presentation);
      const bool main = a.kind == ActionKind::SceneCue;
      accepted = (main || !QuietBanter(*next, story.activeBanter)) && current && (main ? !story.activeScene.empty() : story.activeScene.empty()) &&
          director::Acknowledge({a.target, a.presentationSerial, a.argument, director::Content().cues.at(a.cueId)},
                                context, story, next->presentation); break;
    }
    case ActionKind::CancelScene:
      accepted = story.activeScene == a.target && next->presentation.sceneSerial == a.presentationSerial && director::CancelScene(story, next->presentation); break;
    case ActionKind::CancelBanter:
      accepted = story.activeBanter == a.target && next->presentation.banterSerial == a.presentationSerial && director::CancelBanter(story, next->presentation); break;
    case ActionKind::InspectDocument: case ActionKind::InspectAudio: {
      const auto& block = director::Content().blocks.at(a.target);
      auto facts = story.durableFacts; facts.insert(context.facts.begin(), context.facts.end());
      if (!director::InspectAllowed(a.target, story.difficulty) || (!block.nodes.empty() && !block.nodes.count(a.node)) ||
          !std::includes(facts.begin(), facts.end(), block.requirements.begin(), block.requirements.end())) break;
      if (a.target == "AUD07" && story.witnessedClues.empty()) break;
      auto& inspected = a.kind == ActionKind::InspectDocument ? story.inspectedDocuments : story.inspectedAudio;
      accepted = inspected.insert(a.target).second;
      if (a.kind == ActionKind::InspectDocument && accepted) {
        story.durableFacts.insert("doc" + a.target.substr(3) + "_read");
        for (const auto& [doc, flag] : std::map<std::string, std::string>{{"DOC01","code_a"},{"DOC02","code_b"},{"DOC03","code_c"},{"DOC04","key_known"},{"DOC07","isolation_known"}})
          if (story.inspectedDocuments.count(doc)) story.durableFacts.insert(flag);
        if (story.inspectedDocuments.count("DOC04") && story.inspectedDocuments.count("DOC05")) story.durableFacts.insert("safe_known");
      }
      break;
    }
    case ActionKind::WitnessClue:
      accepted = director::CanWitness(a.target, context, story) && story.witnessedClues.insert(a.target).second; break;
    case ActionKind::RememberFact: accepted = story.durableFacts.insert(a.target).second; break;
    case ActionKind::AdvanceAct:
      accepted = a.argument > story.act && story.seenScenes.count("N03");
      if (accepted) story.act = a.argument;
      break;
    default: break;
  }
  if (!accepted || !next->narrative.RestoreSnapshot(story) || !director::ValidPresentation(story, next->presentation)) return {Status::MissingRequirement, {}};
  next->receipts.emplace(a.eventId, signature); state_.swap(next); return {Status::Applied, {}};
}
std::optional<director::Presentation> CampaignCore::CurrentPresentation(const director::Observation& o, const std::string& locale) const {
  if (Paused() || RescueBusy(*state_,"joe") || !ValidObservation(o)) return std::nullopt;
  const auto story = state_->narrative.CaptureSnapshot();
  if (story.activeScene.empty() && QuietBanter(*state_, story.activeBanter)) return std::nullopt;
  return director::Current(NarrativeContext(o, *state_), state_->narrative.CaptureSnapshot(), state_->presentation, locale);
}
std::vector<director::Cue> CampaignCore::JournalText(const std::string& id, const std::string& locale) const {
  return director::InspectedText(id, state_->narrative.CaptureSnapshot(), locale);
}
std::vector<director::Cue> CampaignCore::EpilogueText(const std::string& locale) const {
  if (Ending() == rules::Ending::None) return {};
  return director::EpilogueText(state_->narrative.CaptureSnapshot(), locale);
}
} // namespace thug::campaign
