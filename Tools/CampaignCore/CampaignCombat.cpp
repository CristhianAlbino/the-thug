#include "CampaignState.h"
#include "CombatIO.h"
#include "CompanionIO.h"
#include <algorithm>
#include <limits>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& list, const std::string& id) {
  for (const auto& value : list) if (value.id == id) return &value;
  return nullptr;
}
bool Zero(hunter::Vec3 p) { return p.x == 0 && p.y == 0 && p.z == 0; }
bool UsesSerial(CombatOp kind) {
  return kind == CombatOp::CompleteReload || kind == CombatOp::CancelReload ||
    kind == CombatOp::ResolveShot || kind == CombatOp::ReleaseThrow ||
    kind == CombatOp::CancelThrow || kind == CombatOp::Detonate;
}
bool Actor(const std::string& id) { return id == "joe" || bool(detail::Person(id)); }
}

CombatTuning CombatProfile(rules::Difficulty mode) {
  CombatTuning p;
  switch (mode) {
    case rules::Difficulty::Easy: p.reloadSeconds = 2; p.oxygenDrain = 5; p.smokeSeconds = 24; break;
    case rules::Difficulty::Normal: break;
    case rules::Difficulty::Hard: p.oxygenDrain = 7; p.smokeSeconds = 18; break;
    case rules::Difficulty::Punitive: p.reloadSeconds = 3; p.oxygenDrain = 8; p.smokeSeconds = 15; break;
    default: throw std::invalid_argument("Unknown combat difficulty");
  }
  return p;
}

CombatRuntimeSnapshot CampaignCore::CombatRuntime() const { return state_->combat; }
bool CampaignCore::WeaponBusy(const State& s,const std::string& actor) const {
  return actor!="joe" && s.combat.companions.count(actor) && bool(s.combat.companions.at(actor).task);
}
std::vector<CombatOp> CampaignCore::CompanionCombatOffers(const std::string& actor,const CombatContext& ctx) const {
  std::vector<CombatOp> offers;
  if(!detail::Person(actor) || !ctx.actualActorNodes.count(actor) || Paused() || Ending()!=rules::Ending::None) return offers;
  for(auto kind:{CombatOp::BeginReload,CombatOp::Fire}) {
    CombatCommand c; c.eventId="combat_offer"; c.kind=kind; c.actor=actor; c.target="rifle";
    c.node=ctx.actualActorNodes.at(actor); c.atSeconds=Now(); c.vector=kind==CombatOp::Fire ? hunter::Vec3{1,0,0} : hunter::Vec3{};
    if(!CombatStaticValid(c) || RescueBusy(*state_,actor)) continue;
    auto candidate=std::make_unique<State>(*state_);
    if(ExecuteCombat(*candidate,c,ctx).status==Status::Applied) offers.push_back(kind);
  }
  return offers;
}
BreathingEffects CampaignCore::Breathing() const {
  const auto& r = state_->combat;
  const auto p = CombatProfile(state_->routes.DifficultyLevel());
  const bool gasp = r.gaspUntil > Now();
  const auto effects = state_->survival.CurrentEffects();
  double loudness = effects.breathingLoudnessMultiplier;
  if (gasp) loudness = std::max(2.5, loudness);
  const double energy = effects.dead || r.holdingBreath ? 0 : std::min(3.0, loudness) / 3;
  return {r.oxygen, p.oxygenCapacity, r.holdingBreath, gasp, energy};
}
std::optional<perception::SoundEvent> CampaignCore::BreathingSound(hunter::Vec3 source) const {
  if (!detail::CombatFinite(source) || Paused() || Ending() != rules::Ending::None || Breathing().energy == 0) return {};
  return perception::SoundEvent{perception::SoundKind::Breathing, source, Now(), Breathing().energy};
}
bool CampaignCore::SmokeOccludes(hunter::Vec3 from, hunter::Vec3 to) const {
  if (!detail::CombatFinite(from) || !detail::CombatFinite(to)) return false;
  const auto p = CombatProfile(state_->routes.DifficultyLevel());
  // Normalize first to avoid squaring long segments and overflowing doubles.
  const double length = detail::CombatDistance(from, to);
  if (!std::isfinite(length)) return false;
  const hunter::Vec3 unit = length == 0 ? hunter::Vec3{} : hunter::Vec3{
    (to.x-from.x)/length, (to.y-from.y)/length, (to.z-from.z)/length};
  for (const auto& [serial, cloud] : state_->combat.smoke) {
    (void)serial;
    double projected = (cloud.center.x-from.x)*unit.x + (cloud.center.y-from.y)*unit.y + (cloud.center.z-from.z)*unit.z;
    projected = std::clamp(projected, 0.0, length);
    const hunter::Vec3 nearest{from.x+projected*unit.x, from.y+projected*unit.y, from.z+projected*unit.z};
    if (detail::CombatDistance(nearest, cloud.center) <= p.smokeRadius) return true;
  }
  return false;
}
bool CampaignCore::TrackThrown(std::uint64_t serial, const std::string& node,
    hunter::Vec3 position, hunter::Vec3 velocity, bool verified) {
  if (!verified || !definition_->nodes.count(node) || !detail::CombatFinite(position) ||
      !detail::CombatFinite(velocity) || Paused() || Ending() != rules::Ending::None) return false;
  const auto found = state_->combat.thrown.find(serial);
  if (found == state_->combat.thrown.end() || Now() > found->second.detonateAt) return false;
  auto next = std::make_unique<State>(*state_);
  auto& item = next->combat.thrown.at(serial);
  item.node = node; item.position = position; item.velocity = velocity;
  state_.swap(next);
  return true;
}
bool CampaignCore::CombatItemAvailable(const State& s, const std::string& item, int quantity) const {
  int reserved = (s.combat.task && s.combat.task->item == item ? 1 : 0) + (s.special.plant && item == "structural_charge" ? 1 : 0);
  for (const auto& [actor, capture] : s.rescue.captures) {
    (void)actor;
    if (capture.release && capture.release->helper == "joe" && capture.release->item == item) ++reserved;
  }
  if(s.strategy.joint && s.strategy.joint->peer=="joe")
    for(const auto& use:definition_->uses) if(use.id==s.strategy.joint->target && use.item==item) reserved+=use.quantity;
  return s.inventory.Quantity(item) - reserved >= quantity;
}
void CampaignCore::ReconcileCombat(State& s) const {
  if (s.survival.CurrentEffects().collapsed || s.survival.CurrentEffects().dead) {
    s.combat.task.reset();
    s.combat.holdingBreath = false;
  }
  ReconcileRescue(s);
  ReconcileStrategy(s);
  const auto story=s.narrative.CaptureSnapshot();
  for(auto& [actor,w]:s.combat.companions) {
    const auto& o=story.outcomes.at(actor); const auto h=s.bodies.at(actor).survival.CurrentEffects();
    if(o.dead || o.captured || o.together || o.escapedAlone || h.dead || h.collapsed || RescueBusy(s,actor)) w.task.reset();
  }
}
bool CampaignCore::AdvanceCombat(State& s, double elapsed, survival::Activity activity) {
  auto& r = s.combat;
  const auto p = CombatProfile(s.routes.DifficultyLevel());
  const double before = s.world.NowSeconds(), now = before + elapsed;
  const auto body = s.survival.CurrentEffects();
  bool forced = false;
  double recovering = elapsed;
  if (r.holdingBreath) {
    if (activity == survival::Activity::Sprint || body.collapsed || body.dead) {
      r.holdingBreath = false;
      r.gaspUntil = before + p.gaspSeconds;
      forced = true;
    } else {
      const double drain = p.oxygenDrain * body.breathingLoudnessMultiplier;
      const double held = std::min(elapsed, r.oxygen / drain);
      r.oxygen = std::max(0.0, r.oxygen - held * drain);
      recovering = elapsed - held;
      if (r.oxygen <= 1e-9) {
        r.oxygen = 0;
        r.holdingBreath = false;
        r.gaspUntil = before + held + p.gaspSeconds;
        forced = true;
      }
    }
  }
  if (!r.holdingBreath) r.oxygen = std::min(p.oxygenCapacity, r.oxygen + recovering * p.oxygenRecovery);
  for (auto it = r.shots.begin(); it != r.shots.end();) {
    if (it->second.expiresAt <= now) it = r.shots.erase(it); else ++it;
  }
  for (auto it = r.smoke.begin(); it != r.smoke.end();) {
    if (it->second.expiresAt <= now) it = r.smoke.erase(it); else ++it;
  }
  return forced;
}

bool CampaignCore::CombatStaticValid(const CombatCommand& c) const {
  if (!Actor(c.actor) || !detail::Token(c.eventId) || !definition_->nodes.count(c.node) ||
      !std::isfinite(c.atSeconds) || c.atSeconds < 0 || c.kind < CombatOp::BeginReload ||
      c.kind > CombatOp::ReleaseBreath || !detail::CombatFinite(c.position) ||
      !detail::CombatFinite(c.vector) || c.contacts.size() > 16 ||
      (UsesSerial(c.kind) ? !c.serial : c.serial != 0) || detail::Signature(c).size() > 4000) return false;
  if (c.kind != CombatOp::Detonate && !c.contacts.empty()) return false;
  if (c.kind != CombatOp::Fire && c.kind != CombatOp::ReleaseThrow && !Zero(c.vector)) return false;
  if (c.kind != CombatOp::Fire && c.kind != CombatOp::ReleaseThrow &&
      c.kind != CombatOp::ResolveShot && c.kind != CombatOp::Detonate && !Zero(c.position)) return false;
  if (c.kind == CombatOp::Fire && std::abs(detail::CombatDistance(c.vector, {}) - 1) > 1e-6) return false;
  switch (c.kind) {
    case CombatOp::BeginReload: case CombatOp::CompleteReload: case CombatOp::CancelReload: case CombatOp::Fire:
      return c.target == "rifle";
    case CombatOp::BeginThrow: case CombatOp::ReleaseThrow: case CombatOp::CancelThrow:
      return c.target == "grenade" || c.target == "smoke";
    case CombatOp::HoldBreath: case CombatOp::ReleaseBreath: return c.actor=="joe" && c.target == "joe";
    case CombatOp::ResolveShot:
      return c.target == "miss" || c.target == "hunter" || Find(definition_->destructibles, c.target);
    case CombatOp::Detonate: {
      if (c.target != "grenade" && c.target != "smoke") return false;
      if (c.target == "smoke" && !c.contacts.empty()) return false;
      std::set<std::string> unique;
      for (const auto& hit : c.contacts) {
        if (!unique.insert(hit.target).second || !definition_->nodes.count(hit.node) ||
            !detail::CombatFinite(hit.position) || hit.injury < survival::Injury::None || hit.injury > survival::Injury::Severe) return false;
        if (Actor(hit.target)) { if (hit.injury == survival::Injury::None) return false; }
        else if ((hit.target != "hunter" && !Find(definition_->destructibles, hit.target)) || hit.injury != survival::Injury::None) return false;
      }
      return true;
    }
  }
  return false;
}

CombatResult CampaignCore::ApplyCombat(const CombatCommand& c, const CombatContext& context) {
  if (!CombatStaticValid(c)) return {Status::InvalidAction};
  const auto old = state_->receipts.find(c.eventId);
  if (old != state_->receipts.end()) {
    CombatCommand saved;
    std::uint64_t order, created;
    return {detail::ParseCombat(old->second, saved, order, created) && detail::Signature(saved) == detail::Signature(c) ? Status::AlreadyApplied : Status::Conflict};
  }
  if (Paused()) return {Status::Paused};
  if (Ending() != rules::Ending::None) return {Status::Finished};
  if (state_->special.phase == SpecialPhase::Collapsing ||
      (state_->special.phase == SpecialPhase::Confronting && Now() >= state_->special.boundaryAt)) return {Status::AwaitingPhysics};
  if (c.atSeconds != Now()) return {Status::InvalidTime};
  if (!context.physicallyVerified || !definition_->nodes.count(context.actualJoeNode)) return {Status::InvalidAction};
  if(context.actualActorNodes.size()>3) return {Status::InvalidAction};
  for(const auto& [actor,node]:context.actualActorNodes) if(!detail::Person(actor) || !definition_->nodes.count(node)) return {Status::InvalidAction};
  if (c.actor=="joe" && !state_->hidingPlace.empty() && context.actualJoeNode != Find(definition_->hides, state_->hidingPlace)->atNode) return {Status::MissingRequirement};
  const bool worldEffect = c.kind == CombatOp::ResolveShot || c.kind == CombatOp::Detonate;
  const auto actorNode=c.actor=="joe" ? context.actualJoeNode :
    context.actualActorNodes.count(c.actor) ? context.actualActorNodes.at(c.actor) : std::string{};
  if (!worldEffect && (actorNode!=c.node || RescueBusy(*state_,c.actor))) return {Status::MissingRequirement};
  std::size_t reserved = detail::CombatReservedReceipts(state_->combat) + detail::SpecialReservedReceipts(state_->special) + detail::RescueReservedReceipts(state_->rescue) + detail::StrategyReservedReceipts(state_->strategy);
  if (c.kind == CombatOp::BeginReload) ++reserved;
  if (c.kind == CombatOp::BeginThrow) reserved += 2;
  const auto& task = c.actor=="joe" ? state_->combat.task : state_->combat.companions.at(c.actor).task;
  if (task && task->serial == c.serial &&
      ((task->kind == CombatOp::BeginReload && (c.kind == CombatOp::CompleteReload || c.kind == CombatOp::CancelReload)) ||
       (task->kind == CombatOp::BeginThrow && task->item == c.target && (c.kind == CombatOp::ReleaseThrow || c.kind == CombatOp::CancelThrow)))) {
    reserved -= c.kind == CombatOp::CancelThrow ? 2 : 1;
  }
  if (c.kind == CombatOp::Detonate && state_->combat.thrown.count(c.serial)) --reserved;
  if (state_->receipts.size() + reserved + 1 > detail::MaxReceipts) return {Status::LogFull};
  auto next = std::make_unique<State>(*state_);
  auto result = ExecuteCombat(*next, c, context);
  if (result.status != Status::Applied) return result;
  const bool creates = c.kind == CombatOp::Fire || c.kind == CombatOp::BeginReload || c.kind == CombatOp::BeginThrow;
  next->receipts.emplace(c.eventId, detail::Receipt(c, ++next->combat.commandSerial, creates ? next->combat.serial : 0));
  ReconcileCombat(*next);
  next->SyncEnding();
  state_.swap(next);
  return result;
}

CombatResult CampaignCore::ExecuteCombat(State& s, const CombatCommand& c, const CombatContext& context) const {
  auto& r = s.combat;
  const auto p = CombatProfile(s.routes.DifficultyLevel());
  const CombatResult denied{Status::MissingRequirement};
  auto& stock=c.actor=="joe" ? s.inventory : s.bodies.at(c.actor).inventory;
  auto& task=c.actor=="joe" ? r.task : r.companions.at(c.actor).task;
  auto& loaded=c.actor=="joe" ? r.loadedRounds : r.companions.at(c.actor).loadedRounds;
  auto& lastShot=c.actor=="joe" ? r.lastShotAt : r.companions.at(c.actor).lastShotAt;
  const auto effects = c.actor=="joe" ? s.survival.CurrentEffects() : s.bodies.at(c.actor).survival.CurrentEffects();
  const bool worldEffect = c.kind == CombatOp::ResolveShot || c.kind == CombatOp::Detonate;
  if (!worldEffect && (effects.dead || effects.collapsed)) return denied;
  if (!worldEffect && c.actor!="joe") {
    const auto story=s.narrative.CaptureSnapshot(); const auto& fate=story.outcomes.at(c.actor);
    if(fate.dead || fate.captured || fate.together || fate.escapedAlone || s.bodies.at(c.actor).task) return denied;
    if(s.special.recruited.count(c.actor) && (s.special.phase==SpecialPhase::Confronting || s.special.phase==SpecialPhase::Opportunity)) return denied;
    if(c.kind==CombatOp::Fire && !context.hunterPerceived && !context.distractionIntended) return denied;
    s.bodies.at(c.actor).lastNode=c.node;
  }
  const auto available=[&](const std::string& item) { return c.actor=="joe" ? CombatItemAvailable(s,item,1) : stock.Quantity(item)>=1; };
  const auto syncAmmo=[&] {
    if(c.actor=="joe") return true;
    auto person=s.Person(*detail::Person(c.actor)).CaptureSnapshot(); int count=0;
    for(const auto& [id,n]:stock.Contents()) if(stock.IsCategory(id,survival::ItemCategory::Ammunition)) count+=n;
    person.traits.ammunition=count; return s.Person(*detail::Person(c.actor)).RestoreSnapshot(person);
  };
  if (!worldEffect && (c.actor=="joe" ? !s.hidingPlace.empty() : !s.bodies.at(c.actor).hidingPlace.empty()) && c.kind != CombatOp::HoldBreath &&
      c.kind != CombatOp::ReleaseBreath && c.kind != CombatOp::CancelReload && c.kind != CombatOp::CancelThrow) return denied;
  CombatResult result{Status::Applied};
  const auto interrupt = [&](double duration) {
    if (c.atSeconds < r.hunterImmuneUntil) return;
    r.hunterBlockedUntil = c.atSeconds + duration;
    r.hunterImmuneUntil = r.hunterBlockedUntil + p.interruptImmunity;
    s.hunterRuntime.task.reset();
    s.hunterRuntime.watchingDoor.clear();
    result.hunterInterrupted = true;
  };
  const auto destroy = [&](const std::string& id, bool explosion, const std::string& node) {
    const auto* d = Find(definition_->destructibles, id);
    if (!d || d->node != node || (explosion ? !d->explosion : !d->gunshot)) return false;
    const auto* current = s.world.Find(id);
    if (!current || !current->present) return false;
    if (current->stateId == d->after) return true; // Another confirmed impact already destroyed it.
    if (current->stateId != d->before) return false;
    auto object = *current; object.stateId = d->after;
    return s.world.SetObject(id, object, c.atSeconds);
  };
  switch (c.kind) {
    case CombatOp::BeginReload: case CombatOp::BeginThrow: {
      if (task) return {Status::Conflict};
      if (r.serial == std::numeric_limits<std::uint64_t>::max()) return {Status::LogFull};
      const bool reload = c.kind == CombatOp::BeginReload;
      const std::string item = reload ? "magazine" : c.target;
      if (!available(item) || (reload && (stock.Quantity("rifle") != 1 || loaded == p.magazineRounds))) return denied;
      task = CombatTask{++r.serial, c.kind, item, c.eventId, c.node, c.atSeconds,
        c.atSeconds + (reload ? p.reloadSeconds : p.throwSeconds)};
      result.task = task;
      break;
    }
    case CombatOp::CompleteReload: case CombatOp::CancelReload: case CombatOp::ReleaseThrow: case CombatOp::CancelThrow: {
      const bool reload = c.kind == CombatOp::CompleteReload || c.kind == CombatOp::CancelReload;
      if (!task || task->serial != c.serial || task->kind != (reload ? CombatOp::BeginReload : CombatOp::BeginThrow) ||
          task->item != (reload ? "magazine" : c.target)) return denied;
      const bool cancel = c.kind == CombatOp::CancelReload || c.kind == CombatOp::CancelThrow;
      if (!cancel) {
        if (!context.animationCompleted || c.atSeconds < task->readyAt ||
            (reload && stock.Quantity("rifle") != 1) ||
            (!reload && r.thrown.size() + r.smoke.size() >= 16) ||
            !stock.Remove(task->item, 1, survival::RemovalReason::Consume)) return denied;
        if (reload) loaded = p.magazineRounds; // Old partially used magazine is discarded, never refilled into stock.
        else {
          ThrownItem item{c.serial, c.eventId, c.target, c.node, c.atSeconds,
            c.atSeconds + (c.target == "grenade" ? p.grenadeFuse : p.smokeFuse), c.position, c.vector};
          item.actor=c.actor;
          r.thrown.emplace(c.serial, item);
          result.thrown = item;
        }
      }
      task.reset();
      if(!syncAmmo()) return denied;
      break;
    }
    case CombatOp::Fire: {
      if (task) return {Status::Conflict};
      if (stock.Quantity("rifle") != 1 || loaded <= 0 || r.shots.size() >= 16 ||
          (lastShot >= 0 && c.atSeconds - lastShot < p.shotInterval)) return denied;
      if (r.serial == std::numeric_limits<std::uint64_t>::max()) return {Status::LogFull};
      --loaded; lastShot = c.atSeconds;
      Shot shot{++r.serial, c.eventId, c.node, c.atSeconds, c.atSeconds + p.shotLifetime, c.position, c.vector};
      shot.actor=c.actor;
      r.shots.emplace(shot.serial, shot);
      result.shot = shot;
      result.sound = perception::SoundEvent{perception::SoundKind::Gunshot, c.position, c.atSeconds, 1};
      break;
    }
    case CombatOp::ResolveShot: {
      const auto shot = r.shots.find(c.serial);
      if (!context.collisionVerified || shot == r.shots.end() || shot->second.actor!=c.actor || c.atSeconds >= shot->second.expiresAt) return denied;
      if (c.target == "hunter") interrupt(p.shotInterrupt);
      else if (c.target != "miss" && !destroy(c.target, false, c.node)) return denied;
      r.shots.erase(shot);
      break;
    }
    case CombatOp::Detonate: {
      const auto item = r.thrown.find(c.serial);
      if (!context.collisionVerified || item == r.thrown.end() || item->second.actor!=c.actor || item->second.item != c.target ||
          c.atSeconds != item->second.detonateAt || item->second.node != c.node ||
          detail::CombatDistance(item->second.position, c.position) > 1e-6) return denied;
      if (c.target == "smoke") {
        r.smoke.emplace(c.serial, SmokeCloud{c.serial, c.eventId, c.node, c.atSeconds, c.atSeconds+p.smokeSeconds, c.position});
      } else {
        for (const auto& hit : c.contacts) {
          if ((hit.target=="joe" || detail::Person(hit.target)) && !RescueLocation(s,hit.target,hit.node)) return denied;
          if (detail::CombatDistance(hit.position, c.position) > p.blastRadius) return denied;
          if (hit.target == "hunter") interrupt(p.blastInterrupt);
          else if (hit.target == "joe") {
            if (hit.node != context.actualJoeNode) return denied;
            // Equal/lower injury is a valid impact even when no worsening occurs.
            s.survival.ApplyInjury(hit.injury);
            s.combat.task.reset();
          } else if (const auto person = detail::Person(hit.target)) {
            const auto story = s.narrative.CaptureSnapshot();
            const auto& o = story.outcomes.at(hit.target);
            if (o.dead || o.captured || o.escapedAlone || o.together) return denied;
            auto& body = s.bodies.at(hit.target);
            if (!body.hidingPlace.empty() && Find(definition_->hides, body.hidingPlace)->atNode != hit.node) return denied;
            body.survival.ApplyInjury(hit.injury);
            r.companions.at(hit.target).task.reset();
            body.lastNode = hit.node;
            body.Stop(TaskStop::Injured);
            s.Person(*person).SetInjured(body.survival.CurrentInjury() != survival::Injury::None, c.atSeconds);
          } else if (!destroy(hit.target, true, hit.node)) return denied;
        }
        result.sound = perception::SoundEvent{perception::SoundKind::Explosion, c.position, c.atSeconds, 1};
      }
      r.thrown.erase(item);
      break;
    }
    case CombatOp::HoldBreath:
      if (r.holdingBreath || r.oxygen < p.holdThreshold || c.atSeconds < r.gaspUntil) return denied;
      r.holdingBreath = true;
      break;
    case CombatOp::ReleaseBreath:
      if (!r.holdingBreath) return denied;
      r.holdingBreath = false;
      if (r.oxygen < p.holdThreshold) r.gaspUntil = c.atSeconds + p.gaspSeconds;
      break;
  }
  return result;
}
} // namespace thug::campaign
