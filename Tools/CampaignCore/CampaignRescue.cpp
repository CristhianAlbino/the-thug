#include "CampaignState.h"
#include "RescueIO.h"
#include "CompanionIO.h"
#include "CombatIO.h"
#include "SpecialIO.h"
#include "InteractionIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
bool At(const RescueContext& c, const std::string& actor, const std::string& node) {
  const auto it = c.actors.find(actor);
  return it != c.actors.end() && it->second.active && it->second.node == node;
}
bool Active(const save::NarrativeSnapshot& s, const std::string& actor) {
  if (actor == "joe") return true;
  const auto& o = s.outcomes.at(actor);
  return !o.dead && !o.captured && !o.together && !o.escapedAlone;
}
}
RescueRuntimeSnapshot CampaignCore::RescueRuntime() const { return state_->rescue; }
bool CampaignCore::RescueBusy(const State& s, const std::string& actor) const {
  for (const auto& [victim, t] : s.rescue.captures) {
    if (victim == actor && (t.phase == CapturePhase::Restraining || t.phase == CapturePhase::Contained)) return true;
    if (t.release && t.release->helper == actor) return true;
  }
  return false;
}
bool CampaignCore::RescueStaticValid(const RescueCommand& c) const {
  if (!detail::Token(c.eventId) || !detail::Person(c.target) || !definition_->nodes.count(c.node) ||
      !std::isfinite(c.atSeconds) || c.atSeconds < 0 || c.kind < RescueOp::BeginContainment ||
      c.kind > RescueOp::FinalizeCapture) return false;
  const bool hunter = c.kind <= RescueOp::CancelContainment || c.kind == RescueOp::FinalizeCapture;
  if (hunter ? c.actor != "hunter" : (c.actor != "joe" && !detail::Person(c.actor))) return false;
  const bool begins = c.kind == RescueOp::BeginContainment || c.kind == RescueOp::BeginRelease || c.kind == RescueOp::BeginSelfRelease;
  if (begins ? c.serial != 0 : c.serial == 0) return false;
  if (c.kind == RescueOp::BeginRelease) return c.actor != c.target && (c.item.empty() || c.item == "window_tool");
  if (!c.item.empty()) return false;
  if (c.kind == RescueOp::BeginSelfRelease) return c.actor == c.target;
  return true;
}
bool CampaignCore::RescueLocation(const State& s, const std::string& actor, const std::string& node) const {
  for (const auto& [victim, t] : s.rescue.captures) {
    if ((victim == actor && (t.phase == CapturePhase::Restraining || t.phase == CapturePhase::Contained)) ||
        (t.release && t.release->helper == actor)) if (t.node != node) return false;
  }
  return true;
}
std::uint64_t CampaignCore::RescueHarmCount(const State& s, const std::string& actor, double before) const {
  std::uint64_t count = 0;
  for (const auto& [id, signature] : s.receipts) {
    (void)id;
    double at = 0; bool harm = false;
    if (signature.rfind("C ", 0) == 0) {
      CompanionCommand c; if (!detail::ParseCompanion(signature, c)) continue;
      at = c.atSeconds; harm = (c.actor == actor && (c.kind == CompanionOp::Injury ||
          (c.kind == CompanionOp::Outcome && c.argument == static_cast<int>(CompanionOutcome::Dead)))) ||
          (c.target == actor && (c.kind == CompanionOp::Treat || c.kind == CompanionOp::Stabilize)) ||
          (actor == "joe" && c.kind == CompanionOp::Complete && c.argument == static_cast<int>(companion::Goal::HelpJoe));
    } else if (signature.rfind("W ", 0) == 0) {
      CombatCommand c; std::uint64_t order, created;
      if (!detail::ParseCombat(signature, c, order, created)) continue;
      at = c.atSeconds;
      if (c.kind == CombatOp::Detonate) for (const auto& hit : c.contacts) if (hit.target == actor) harm = true;
    } else if (signature.rfind("S ", 0) == 0) {
      SpecialCommand c; std::uint64_t order;
      if (!detail::ParseSpecial(signature, c, order)) continue;
      at = c.atSeconds;
      const auto fate = c.fates.find(actor);
      harm = (actor == "joe" && (c.kind == SpecialOp::ResolveConfrontation || c.kind == SpecialOp::ResolveCollapse)) ||
          (fate != c.fates.end() && (fate->second == SpecialFate::Injured || fate->second == SpecialFate::Dead));
    } else if (signature.rfind("I ", 0) == 0) {
      InteractionCommand c; std::uint64_t order; bool accepted;
      if (!detail::ParseInteraction(signature, c, order, accepted)) continue;
      at = c.atSeconds; harm = actor == "joe" && c.kind == InteractionOp::Gesture && c.argument == 1 && accepted;
    } else {
      Action c; if (!detail::ParseSignature(signature, c)) continue;
      at = c.atSeconds; harm = actor == "joe" && (c.kind == ActionKind::Injury ||
          c.kind == ActionKind::Treat || c.kind == ActionKind::Stabilize);
    }
    if (harm && (before < 0 || at < before)) ++count;
  }
  return count;
}
void CampaignCore::ReconcileRescue(State& s) const {
  const auto story = s.narrative.CaptureSnapshot();
  for (auto& [victim, t] : s.rescue.captures) if (t.release) {
    auto& a = *t.release;
    const auto helper = a.helper == "joe" ? s.survival.CurrentEffects() : s.bodies.at(a.helper).survival.CurrentEffects();
    const auto target = s.bodies.at(victim).survival.CurrentEffects();
    a.interrupted = s.routes.Result() != rules::Ending::None || !Active(story, victim) || !Active(story, a.helper) ||
        helper.dead || helper.collapsed || target.dead || target.collapsed ||
        RescueHarmCount(s, a.helper) != a.helperHarm || RescueHarmCount(s, victim) != a.targetHarm;
  }
}
std::vector<RescueOffer> CampaignCore::RescueOffers(const std::string& actor, const RescueContext& context) const {
  std::vector<RescueOffer> offers;
  if (actor != "joe" && !detail::Person(actor)) return offers;
  if (Paused() || Ending() != rules::Ending::None || !definition_->nodes.count(context.actualHunterNode) || context.actors.size()>4) return offers;
  for (const auto& [name,a] : context.actors)
    if ((name!="joe" && !detail::Person(name)) || !definition_->nodes.count(a.node)) return offers;
  for (const auto& [target, t] : state_->rescue.captures) {
    if (t.phase != CapturePhase::Contained || t.release) continue;
    for (const auto& item : {"", "window_tool"}) {
      if (actor == target && *item) continue;
      RescueCommand c; c.eventId = "offer"; c.actor = actor; c.target = target; c.node = t.node;
      c.atSeconds = Now(); c.item = item; c.kind = actor == target ? RescueOp::BeginSelfRelease : RescueOp::BeginRelease;
      auto candidate = std::make_unique<State>(*state_);
      if (ExecuteRescue(*candidate, c, context) == Status::Applied)
        offers.push_back({c.kind, target, item, actor == target ? SelfReleaseSeconds : *item ? ToolReleaseSeconds : ManualReleaseSeconds});
    }
  }
  return offers;
}
Status CampaignCore::ApplyRescue(const RescueCommand& c, const RescueContext& context) {
  if (!RescueStaticValid(c)) return Status::InvalidAction;
  const auto existing = state_->receipts.find(c.eventId);
  if (existing != state_->receipts.end()) {
    RescueCommand saved; std::uint64_t order, helper, target; bool witnessed;
    return detail::ParseRescue(existing->second, saved, order, witnessed, helper, target) &&
        existing->second == detail::Signature(c, order, witnessed, helper, target) ? Status::AlreadyApplied : Status::Conflict;
  }
  if (Paused()) return Status::Paused;
  if (Ending() != rules::Ending::None && c.kind != RescueOp::CancelRelease) return Status::Finished;
  if (c.atSeconds != Now()) return Status::InvalidTime;
  if (!c.physicallyVerified || context.actors.size() > 4 ||
      (!context.actualHunterNode.empty() && !definition_->nodes.count(context.actualHunterNode))) return Status::InvalidAction;
  for (const auto& [actor, a] : context.actors)
    if ((actor != "joe" && !detail::Person(actor)) || !definition_->nodes.count(a.node)) return Status::InvalidAction;
  if (state_->special.phase == SpecialPhase::Collapsing ||
      (state_->special.phase == SpecialPhase::Confronting && Now() >= state_->special.boundaryAt)) return Status::AwaitingPhysics;
  if (state_->rescue.serial == std::numeric_limits<std::uint64_t>::max()) return Status::LogFull;
  auto next = std::make_unique<State>(*state_);
  const auto result = ExecuteRescue(*next, c, context);
  if (result != Status::Applied) return result;
  // Admit against the resulting reservations, including this command's receipt.
  if (next->receipts.size() + 1 + detail::CombatReservedReceipts(next->combat) +
      detail::SpecialReservedReceipts(next->special) + detail::RescueReservedReceipts(next->rescue) > detail::MaxReceipts) return Status::LogFull;
  const bool witnessed = c.actor == "joe" || (context.joeWitnessedAction && At(context, "joe", c.node));
  next->receipts.emplace(c.eventId, detail::Signature(c, ++next->rescue.serial, witnessed,
      c.actor == "hunter" ? 0 : RescueHarmCount(*next, c.actor), RescueHarmCount(*next, c.target)));
  next->SyncEnding(); ReconcileCombat(*next); ReconcileRescue(*next);
  state_.swap(next);
  return Status::Applied;
}
Status CampaignCore::ExecuteRescue(State& s, const RescueCommand& c, const RescueContext& context) const {
  const auto denied = Status::MissingRequirement;
  auto story = s.narrative.CaptureSnapshot();
  auto& captures = s.rescue.captures;
  auto found = captures.find(c.target);
  auto& victim = s.bodies.at(c.target);
  const auto viable = [&](const std::string& actor) {
    const auto& health = actor == "joe" ? s.survival : s.bodies.at(actor).survival;
    return Active(story, actor) && !health.CurrentEffects().dead && !health.CurrentEffects().collapsed;
  };
  const auto hunterContact = [&] {
    const auto track = s.hunterRuntime.actors.find(c.target);
    return context.actualHunterNode == c.node && context.contactVerified && At(context, c.target, c.node) &&
        s.special.phase != SpecialPhase::Confronting &&
        !(s.special.phase == SpecialPhase::Opportunity && c.atSeconds < s.special.boundaryAt) &&
        s.combat.hunterBlockedUntil <= c.atSeconds && track != s.hunterRuntime.actors.end() &&
        track->second.visible && track->second.node == c.node;
  };
  if (c.kind == RescueOp::BeginContainment) {
    if (!Active(story,c.target) || victim.survival.CurrentEffects().dead || !hunterContact() || !victim.hidingPlace.empty() ||
        !RescueLocation(s,c.target,c.node) || (found != captures.end() && found->second.phase != CapturePhase::Released)) return denied;
    // One physical restraint at a time; no remote or simultaneous Hunter hands.
    for (const auto& [actor, t] : captures) { (void)actor; if (t.phase == CapturePhase::Restraining) return denied; }
    for (auto& [actor, t] : captures) { (void)actor; if (t.release && t.release->helper==c.target) t.release.reset(); }
    victim.Stop(TaskStop::Cancelled); victim.lastNode = c.node;
    s.hunterRuntime.task.reset(); s.hunterRuntime.watchingDoor.clear();
    captures[c.target] = {CapturePhase::Restraining, c.node, c.eventId, s.rescue.serial + 1, c.atSeconds, -1, {}};
    return Status::Applied;
  }
  if (found == captures.end() || found->second.node != c.node) return denied;
  auto& t = found->second;
  if (c.kind == RescueOp::CancelRelease) {
    if (!t.release || t.release->serial != c.serial || t.release->helper != c.actor) return denied;
    t.release.reset(); return Status::Applied;
  }
  if ((!Active(story,c.target) || victim.survival.CurrentEffects().dead) && c.kind!=RescueOp::CancelContainment) return denied;
  if (c.kind == RescueOp::FinishContainment || c.kind == RescueOp::CancelContainment) {
    if (t.phase != CapturePhase::Restraining || c.serial != t.serial) return denied;
    if (c.kind == RescueOp::CancelContainment) {
      if (!context.contactLostVerified && s.combat.hunterBlockedUntil <= c.atSeconds) return denied;
      t.phase = CapturePhase::Released;
    } else {
      if (!hunterContact() || !context.continuousContactVerified || !context.actionCompleted ||
          c.atSeconds < t.startedAt + RestrainSeconds) return denied;
      t.phase = CapturePhase::Contained; t.containedAt = c.atSeconds;
    }
    return Status::Applied;
  }
  if (t.phase != CapturePhase::Contained) return denied;
  if (c.kind == RescueOp::FinalizeCapture) {
    if (c.serial != t.serial || !hunterContact() || !context.transferCompleted || !context.actionCompleted ||
        c.atSeconds < t.containedAt + CaptureTransferSeconds) return denied;
    t.phase = CapturePhase::Finalized; t.release.reset();
    auto& outcome = story.outcomes.at(c.target); outcome.captured = true; outcome.lastContactNode = c.node;
    victim.Stop(TaskStop::Outcome);
    if (!s.narrative.RestoreSnapshot(story)) return denied;
    return Status::Applied;
  }
  const bool self = c.kind == RescueOp::BeginSelfRelease || (t.release && t.release->self);
  if (!viable(c.target) || !viable(c.actor) || !At(context, c.actor, c.node) || !At(context, c.target, c.node) ||
      !context.contactVerified || !context.openingVerified || context.actualHunterNode.empty() ||
      (context.actualHunterNode == c.node && s.combat.hunterBlockedUntil <= c.atSeconds)) return denied;
  if (c.kind == RescueOp::CompleteRelease) {
    if (!t.release || t.release->serial != c.serial || t.release->helper != c.actor ||
        t.release->interrupted || !context.continuousContactVerified || !context.actionCompleted ||
        c.atSeconds < t.release->readyAt) return denied;
    auto& inventory = c.actor == "joe" ? s.inventory : s.bodies.at(c.actor).inventory;
    if (!t.release->item.empty() && !inventory.Remove(t.release->item, 1, survival::RemovalReason::Consume)) return denied;
    t.release.reset(); t.phase = CapturePhase::Released;
    return Status::Applied;
  }
  if (t.release || (!self && RescueBusy(s, c.actor))) return denied;
  if (c.actor == "joe") {
    if (!s.hidingPlace.empty() || s.combat.task || s.special.plant ||
        s.special.phase == SpecialPhase::Confronting || s.special.phase == SpecialPhase::Opportunity) return denied;
  } else {
    const auto& b = s.bodies.at(c.actor); const auto& traits = s.Person(*detail::Person(c.actor)).CurrentTraits();
    if (s.special.recruited.count(c.actor) &&
        (s.special.phase==SpecialPhase::Confronting || s.special.phase==SpecialPhase::Opportunity)) return denied;
    if (b.task || !b.hidingPlace.empty() || b.survival.CurrentInjury() > survival::Injury::Light ||
        traits.fear + traits.stress >= 180 || (!self && (traits.trust < 35 || traits.loyalty < 30))) return denied;
  }
  if (!c.item.empty()) {
    if (c.actor == "joe" && !CombatItemAvailable(s, c.item, 1)) return denied;
    const auto& bag = c.actor == "joe" ? s.inventory : s.bodies.at(c.actor).inventory;
    if (bag.Quantity(c.item) < 1) return denied;
  }
  t.release = ReleaseTask{s.rescue.serial + 1, RescueHarmCount(s, c.actor), RescueHarmCount(s, c.target),
      c.actor, c.item, c.eventId, c.atSeconds, c.atSeconds + (self ? SelfReleaseSeconds : c.item.empty() ? ManualReleaseSeconds : ToolReleaseSeconds), self, false};
  if (c.actor != "joe") s.bodies.at(c.actor).lastNode = c.node;
  return Status::Applied;
}
} // namespace thug::campaign
