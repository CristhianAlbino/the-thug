#include "CampaignState.h"
#include "SpecialIO.h"
#include "CompanionIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& entries, const std::string& id) {
  for (const auto& entry : entries) if (entry.id == id) return &entry;
  return nullptr;
}
bool Active(const save::NarrativeSnapshot& story, const std::string& actor) {
  const auto& o = story.outcomes.at(actor);
  return !o.dead && !o.captured && !o.escapedAlone && !o.together;
}
bool Present(const SpecialContext& context, const std::string& actor, const std::string& node) {
  const auto found = context.participants.find(actor);
  return found != context.participants.end() && found->second.active && found->second.node == node;
}
void SyncBody(companion::CompanionCore& person, const CompanionBody& body) {
  auto snapshot = person.CaptureSnapshot();
  snapshot.traits.ammunition = body.inventory.Quantity("magazine");
  snapshot.traits.injured = body.survival.CurrentInjury() != survival::Injury::None;
  if (!person.RestoreSnapshot(snapshot)) throw std::logic_error("Invalid rare finale body");
}
}

SpecialRuntimeSnapshot CampaignCore::SpecialRuntime() const { return state_->special; }
bool CampaignCore::SpecialStaticValid(const SpecialCommand& c) const {
  if (!detail::Token(c.eventId) || !detail::Token(c.target) || !definition_->nodes.count(c.node) ||
      !std::isfinite(c.atSeconds) || c.atSeconds < 0 || c.kind < SpecialOp::Recruit || c.kind > SpecialOp::WitnessAppearance ||
      c.fates.size() > 3 || (c.kind != SpecialOp::ResolveConfrontation && c.kind != SpecialOp::ResolveCollapse && c.argument) || c.argument < 0 || c.argument > 1) return false;
  const bool completion = c.kind == SpecialOp::CompletePlant || c.kind == SpecialOp::CancelPlant;
  if (completion != bool(c.serial)) return false;
  for (const auto& [actor, fate] : c.fates) {
    if (!detail::Person(actor) || fate < SpecialFate::Injured || fate > SpecialFate::Escaped) return false;
    if (c.kind == SpecialOp::ConfrontationEscape ? fate != SpecialFate::Escaped :
        (c.kind != SpecialOp::ResolveConfrontation && c.kind != SpecialOp::ResolveCollapse) || fate == SpecialFate::Escaped) return false;
  }
  if (c.kind == SpecialOp::WitnessAppearance) return c.fates.empty() && Find(definition_->appearances, c.target);
  if (!definition_->finale) return false;
  const auto& d = *definition_->finale;
  switch (c.kind) {
    case SpecialOp::Recruit: return bool(detail::Person(c.target)) && c.node == d.confrontationNode && c.fates.empty();
    case SpecialOp::AcquireSupply: { const auto* p = Find(d.supplies, c.target); return p && c.node == p->node && c.fates.empty(); }
    case SpecialOp::BeginPlant: case SpecialOp::CompletePlant: case SpecialOp::CancelPlant: {
      const auto* p = Find(d.sites, c.target);
      return p && (c.kind == SpecialOp::CancelPlant || c.node == p->node) && c.fates.empty();
    }
    case SpecialOp::StartConfrontation: case SpecialOp::ResolveConfrontation:
      return c.target == "confrontation" && c.node == d.confrontationNode && (c.kind != SpecialOp::StartConfrontation || c.fates.empty());
    case SpecialOp::ConfrontationEscape: return c.target == "confrontation" && c.node == d.confrontationExit;
    case SpecialOp::DetonateCollapse: return c.target == "collapse" && c.node == d.detonatorNode && c.fates.empty();
    case SpecialOp::ResolveCollapse: return c.target == "collapse" && (!c.argument || c.node == d.collapseShelter);
    default: return false;
  }
}

Status CampaignCore::ApplySpecial(const SpecialCommand& c, const SpecialContext& context) {
  if (!SpecialStaticValid(c)) return Status::InvalidAction;
  const auto receipt = state_->receipts.find(c.eventId);
  if (receipt != state_->receipts.end()) {
    SpecialCommand old; std::uint64_t order = 0;
    return detail::ParseSpecial(receipt->second, old, order) && receipt->second == detail::SpecialSignature(c, order)
        ? Status::AlreadyApplied : Status::Conflict;
  }
  if (Paused()) return Status::Paused;
  if (Ending() != rules::Ending::None) return Status::Finished;
  if (c.atSeconds != Now()) return Status::InvalidTime;
  if (!context.physicallyVerified || !context.actionCompleted || context.actualJoeNode != c.node ||
      context.participants.size() > 3 || (!c.fates.empty() && !context.fatesWitnessed)) return Status::InvalidAction;
  for (const auto& [actor, view] : context.participants)
    if (!detail::Person(actor) || !definition_->nodes.count(view.node)) return Status::InvalidAction;
  if ((!state_->hidingPlace.empty() && (c.kind != SpecialOp::CancelPlant ||
       Find(definition_->hides, state_->hidingPlace)->atNode != c.node)) ||
      (state_->survival.CurrentEffects().collapsed && c.kind != SpecialOp::CancelPlant && c.kind != SpecialOp::ResolveCollapse)) return Status::MissingRequirement;
  const auto& r = state_->special;
  if (r.phase == SpecialPhase::Collapsing && c.kind != SpecialOp::ResolveCollapse) return Status::AwaitingPhysics;
  if (r.phase == SpecialPhase::Confronting && Now() >= r.boundaryAt && c.kind != SpecialOp::ResolveConfrontation) return Status::AwaitingPhysics;
  const bool finishing = c.kind == SpecialOp::CompletePlant || c.kind == SpecialOp::CancelPlant ||
      c.kind == SpecialOp::ResolveConfrontation || c.kind == SpecialOp::ResolveCollapse || c.kind == SpecialOp::ConfrontationEscape;
  const auto reserved = detail::CombatReservedReceipts(state_->combat) + detail::SpecialReservedReceipts(r) + detail::RescueReservedReceipts(state_->rescue);
  const std::size_t extra = c.kind == SpecialOp::StartConfrontation ? 2 : c.kind == SpecialOp::BeginPlant || c.kind == SpecialOp::DetonateCollapse ? 1 : 0;
  if (state_->receipts.size() + reserved + extra - (finishing && reserved ? 1 : 0) >= detail::MaxReceipts) return Status::LogFull;
  auto next = std::make_unique<State>(*state_);
  const auto result = ExecuteSpecial(*next, c, context);
  if (result != Status::Applied) return result;
  next->receipts.emplace(c.eventId, detail::SpecialSignature(c, ++next->special.commandSerial));
  next->SyncEnding(); ReconcileCombat(*next);
  state_.swap(next);
  return Status::Applied;
}

Status CampaignCore::ExecuteSpecial(State& s, const SpecialCommand& c, const SpecialContext& context) {
  auto& r = s.special;
  auto story = s.narrative.CaptureSnapshot();
  const auto mode = s.routes.DifficultyLevel();
  const auto deny = Status::MissingRequirement;
  for (const auto& [actor,fate] : c.fates)
    if (!RescueLocation(s,actor,c.node) || (RescueBusy(s,actor) &&
        fate!=SpecialFate::Injured && fate!=SpecialFate::Dead)) return deny;
  if (RescueBusy(s,"joe") && c.kind!=SpecialOp::ResolveCollapse && c.kind!=SpecialOp::ResolveConfrontation) return deny;
  if (c.kind == SpecialOp::WitnessAppearance) {
    const auto& a = *Find(definition_->appearances, c.target);
    if (!(a.modes & (1u << static_cast<int>(mode))) || story.act < a.minimumAct ||
        c.node != a.node || r.appearanceTicket >= a.tickets || !context.appearancePerceived || !r.appearances.insert(a.id).second) return deny;
    return Status::Applied;
  }
  const auto& d = *definition_->finale;
  const bool confrontation = c.kind <= SpecialOp::ConfrontationEscape;
  if (mode != (confrontation ? rules::Difficulty::Easy : rules::Difficulty::Punitive) || story.act != 5) return deny;
  const auto viable = [&](const std::string& actor) {
    const auto& body = s.bodies.at(actor);
    return !RescueBusy(s,actor) && Active(story, actor) && Present(context, actor, c.node) && !body.task && body.hidingPlace.empty() &&
        !body.survival.CurrentEffects().collapsed && body.survival.CurrentInjury() < survival::Injury::Moderate && s.Person(*detail::Person(actor)).CurrentTraits().trust >= d.minimumTrust;
  };
  switch (c.kind) {
    case SpecialOp::Recruit:
      if (r.phase != SpecialPhase::Idle || !viable(c.target) || !r.recruited.insert(c.target).second) return deny;
      break;
    case SpecialOp::StartConfrontation:
      if (r.phase != SpecialPhase::Idle || r.recruited.size() < static_cast<std::size_t>(d.minimumCompanions) ||
          s.combat.task || !s.combat.thrown.empty() || !context.warningPresented || s.inventory.Quantity("rifle") != 1 ||
          !CombatItemAvailable(s, "magazine", 1) || !CombatItemAvailable(s, "smoke", 1)) return deny;
      for (const auto& actor : r.recruited) if (!viable(actor) || s.bodies.at(actor).inventory.Quantity("magazine") < 1) return deny;
      s.inventory.Remove("magazine", 1, survival::RemovalReason::Consume);
      s.inventory.Remove("smoke", 1, survival::RemovalReason::Consume);
      for (const auto& actor : r.recruited) {
        auto& body = s.bodies.at(actor); body.inventory.Remove("magazine", 1, survival::RemovalReason::Consume);
        body.lastNode = c.node; SyncBody(s.Person(*detail::Person(actor)), body);
      }
      r.phase = SpecialPhase::Confronting; r.boundaryAt = c.atSeconds + d.confrontationSeconds;
      break;
    case SpecialOp::ResolveConfrontation:
      if (r.phase != SpecialPhase::Confronting || c.atSeconds != r.boundaryAt ||
          (c.argument && (!context.hunterIncapacitatedVerified || context.actualHunterNode != c.node))) return deny;
      for (const auto& [actor, fate] : c.fates) { (void)fate; if (!r.recruited.count(actor)) return deny; }
      r.phase = c.argument ? SpecialPhase::Opportunity : SpecialPhase::Failed;
      r.boundaryAt = c.argument ? c.atSeconds + d.opportunitySeconds : -1;
      if (c.argument) { s.hunterRuntime.task.reset(); s.hunterRuntime.watchingDoor.clear(); s.survival.ApplyInjury(survival::Injury::Moderate); }
      break;
    case SpecialOp::ConfrontationEscape:
      if (r.phase != SpecialPhase::Opportunity || c.atSeconds >= r.boundaryAt || !context.movementCompleted) return deny;
      for (const auto& [actor, fate] : c.fates) { (void)fate; if (!r.recruited.count(actor) || !Active(story, actor) || !Present(context, actor, c.node)) return deny; }
      if (!s.routes.Apply(rules::Event::CoordinatedEscapeConfirmed, c.atSeconds)) return deny;
      story.durableFacts.insert("rare_confrontation"); r.phase = SpecialPhase::Escaped; r.boundaryAt = -1;
      break;
    case SpecialOp::AcquireSupply: {
      const auto& p = *Find(d.supplies, c.target); const auto* object = s.world.Find(p.id);
      if (!object || !object->present || object->stateId != "available" || !s.inventory.Add(p.item, p.quantity) || !r.supplies.insert(p.id).second) return deny;
      auto collected = *object; collected.present = false; collected.stateId = "collected";
      if (!s.world.SetObject(p.id, collected, c.atSeconds)) return deny;
      break;
    }
    case SpecialOp::BeginPlant:
      if (r.plant || r.planted.count(c.target) || r.phase != SpecialPhase::Idle || s.combat.task ||
          !CombatItemAvailable(s, "structural_charge", 1)) return deny;
      r.plant = PlantTask{++r.plantSerial, c.target, c.eventId, c.atSeconds, c.atSeconds + d.plantSeconds};
      break;
    case SpecialOp::CompletePlant: case SpecialOp::CancelPlant:
      if (!r.plant || r.plant->serial != c.serial || r.plant->site != c.target) return deny;
      if (c.kind == SpecialOp::CompletePlant) {
        if (c.atSeconds < r.plant->readyAt || !s.inventory.Remove("structural_charge", 1, survival::RemovalReason::Consume)) return deny;
        auto object = *s.world.Find(c.target); object.stateId = "planted";
        if (!s.world.SetObject(c.target, object, c.atSeconds) || !r.planted.insert(c.target).second) return deny;
      }
      r.plant.reset();
      break;
    case SpecialOp::DetonateCollapse:
      if (r.phase != SpecialPhase::Idle || r.plant || r.planted.size() != d.sites.size() || s.combat.task ||
          !s.combat.thrown.empty() || !context.warningPresented || !CombatItemAvailable(s, "structural_detonator", 1) ||
          !s.inventory.Remove("structural_detonator", 1, survival::RemovalReason::Consume)) return deny;
      r.phase = SpecialPhase::Collapsing; r.boundaryAt = c.atSeconds + d.collapseSeconds;
      break;
    case SpecialOp::ResolveCollapse:
      if (r.phase != SpecialPhase::Collapsing || c.atSeconds != r.boundaryAt ||
          (c.argument ? !context.joeSurvivedCollapse || !context.movementCompleted || context.joeDeathVerified : !context.joeDeathVerified || context.joeSurvivedCollapse)) return deny;
      if (!s.routes.Apply(c.argument ? rules::Event::CollapseSurvivalConfirmed : rules::Event::JoeDied, c.atSeconds)) return deny;
      if (c.argument) { s.survival.ApplyInjury(survival::Injury::Severe); s.survival.Stabilize(); story.durableFacts.insert("rare_destruction"); }
      else {
        auto health = s.survival.CaptureSnapshot(); health.injury = survival::Injury::Severe;
        health.bloodLoss = health.tuning.fatalBloodLoss; health.bleeding = health.collapsed = health.dead = true;
        if (!s.survival.RestoreSnapshot(health)) return deny;
      }
      r.phase = c.argument ? SpecialPhase::Destroyed : SpecialPhase::FatalCollapse; r.boundaryAt = -1;
      for (const auto& site : d.sites) { auto object = *s.world.Find(site.id); object.stateId = "destroyed"; s.world.SetObject(site.id, object, c.atSeconds); }
      break;
    default: return deny;
  }
  for (const auto& [actor, fate] : c.fates) {
    if (!Active(story, actor) || !Present(context, actor, c.node)) return deny;
    auto& body = s.bodies.at(actor); auto& outcome = story.outcomes.at(actor);
    if (!body.hidingPlace.empty()) return deny;
    if (fate == SpecialFate::Injured) body.survival.ApplyInjury(survival::Injury::Moderate);
    else if (fate == SpecialFate::Dead) {
      auto health = body.survival.CaptureSnapshot(); health.injury = survival::Injury::Severe;
      health.bloodLoss = health.tuning.fatalBloodLoss; health.bleeding = health.collapsed = health.dead = true;
      if (!body.survival.RestoreSnapshot(health)) return deny;
      outcome.dead = true;
    } else if (fate == SpecialFate::Captured) outcome.captured = true;
    else outcome.together = true;
    body.lastNode = c.node; body.Stop(fate == SpecialFate::Injured ? TaskStop::Injured : TaskStop::Outcome);
    if (fate != SpecialFate::Injured) outcome.lastContactNode = c.node;
    SyncBody(s.Person(*detail::Person(actor)), body);
  }
  // Together outcomes are valid only after SyncEnding establishes Joe's escape.
  story.joeEscaped = s.routes.Result() == rules::Ending::CoordinatedEscape || story.joeEscaped;
  return s.narrative.RestoreSnapshot(story) ? Status::Applied : deny;
}
} // namespace thug::campaign
