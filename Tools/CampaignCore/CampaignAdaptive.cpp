#include "CampaignState.h"
#include "StrategyIO.h"
#include "HunterIO.h"
#include "CompanionIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& list,const std::string& id) {
  for(const auto& value:list) if(value.id==id) return &value;
  return nullptr;
}
std::pair<std::string,std::string> Step(const HunterPlan& p,const WorldDefinition& d) {
  if(p.kind==StrategyOp::BeginTrap && p.step==0) return {p.auxiliary,d.circuitNodes.at(p.auxiliary)};
  return {p.target,p.destination};
}
}
bool CampaignCore::AdaptiveStaticValid(const StrategyCommand& c) const {
  if(!detail::Token(c.eventId) || c.actor!="hunter" || !definition_->nodes.count(c.node) ||
      !std::isfinite(c.atSeconds) || c.atSeconds<0 || c.position.x || c.position.y || c.position.z || !c.item.empty() ||
      c.kind<StrategyOp::NoticeSmoke || c.kind>StrategyOp::CancelPlan) return false;
  if(c.kind!=StrategyOp::BeginTrap && !c.reference.empty()) return false;
  if(c.kind>=StrategyOp::ArrivePlan && !c.destination.empty()) return false;
  switch(c.kind) {
    case StrategyOp::NoticeSmoke: return c.serial>0 && definition_->nodes.count(c.target) && c.destination.empty() && !c.argument;
    case StrategyOp::BeginTrap: {
      const auto* d=Find(definition_->doors,c.target);
      return !c.serial && !c.argument && definition_->circuitNodes.count(c.reference) && d && (c.destination==d->fromNode || c.destination==d->toNode);
    }
    case StrategyOp::BeginSmokeWatch: case StrategyOp::BeginRetreat:
      return !c.serial && !c.argument && definition_->nodes.count(c.target) && definition_->nodes.count(c.destination) &&
        (c.kind!=StrategyOp::BeginRetreat || c.target==c.destination);
    case StrategyOp::ArrivePlan: case StrategyOp::CompletePlanStep: return c.serial>0 && detail::Token(c.target) && c.argument>=0 && c.argument<=2;
    case StrategyOp::CancelPlan: return c.serial>0 && detail::Token(c.target) && !c.argument;
    default: return false;
  }
}
void CampaignCore::ReconcileAdaptive(State& s) const {
  if(!s.strategy.hunterPlan) return;
  auto& p=*s.strategy.hunterPlan;
  p.interrupted=p.interrupted || s.routes.Result()!=rules::Ending::None || RescueHarmCount(s,"hunter")!=p.harm;
  // A plan starts after previous sensor instants. Any newer identified contact
  // interrupts that plan, even if line of sight is lost again before loading.
  for(const auto& [actor,t]:s.hunterRuntime.actors) { (void)actor; if(t.estimate.observedAtSeconds>=p.startedAt) p.interrupted=true; }
}
std::vector<HunterPlanOffer> CampaignCore::HunterPlanOffers(const StrategyContext& ctx) const {
  if(Paused() || Ending()!=rules::Ending::None) return {};
  return HunterPlanOffers(*state_,ctx);
}
std::vector<HunterPlanOffer> CampaignCore::HunterPlanOffers(const State& s,const StrategyContext& ctx) const {
  std::vector<HunterPlanOffer> offers;
  const auto& r=s.strategy; const double now=s.world.NowSeconds(); const auto p=HunterProfile(s.routes.DifficultyLevel());
  if(!definition_->nodes.count(ctx.actualHunterNode) || r.hunterPlan || r.question || s.hunterRuntime.task ||
      now<s.combat.hunterBlockedUntil || s.special.phase==SpecialPhase::Confronting || s.special.phase==SpecialPhase::Opportunity ||
      s.special.phase==SpecialPhase::Collapsing || (r.lastPlanAt>=0 && now-r.lastPlanAt<p.ambushCooldown)) return offers;
  for(const auto& [actor,t]:s.hunterRuntime.actors) {
    (void)actor; if(t.estimate.observedAtSeconds>=now || (t.visible && now-t.estimate.observedAtSeconds<=p.visualLease)) return offers;
  }
  for(const auto& [actor,c]:s.rescue.captures) { (void)actor; if(c.phase==CapturePhase::Restraining) return offers; }
  for(const auto& n:ctx.reachableNodes) if(!definition_->nodes.count(n)) return {};
  const auto seen=s.evidence.CaptureSnapshot().lastSeen;
  if(s.hunterRuntime.aggression>=p.environmentThreshold) {
    for(const auto& d:definition_->doors) {
      if(!ctx.safeToAlter.count(d.id) || !seen.count(d.id) || (seen.at(d.id)!="open" && seen.at(d.id)!="forced_open")) continue;
      std::set<double> times;
      for(const auto& [id,signature]:s.receipts) if(signature.rfind("H ",0)==0) {
        (void)id; HunterCommand c; std::optional<HunterTask> plan;
        if(detail::ParseHunter(signature,c,plan) && c.kind==HunterOp::NoticePassage && c.target==d.id && now-c.atSeconds<=120) times.insert(c.atSeconds);
      }
      if(times.size()<2 || *times.rbegin()-*times.begin()<15) continue;
      for(const auto& [circuit,node]:definition_->circuitNodes) if(ctx.reachableNodes.count(node) && ctx.safeToAlter.count(circuit) && seen.count(circuit) && seen.at(circuit)=="on")
        for(const auto& side:{d.fromNode,d.toNode}) if(ctx.reachableNodes.count(side)) offers.push_back({StrategyOp::BeginTrap,d.id,side,circuit,p.ambushWeight});
    }
  }
  for(const auto& [node,pattern]:r.smokePatterns) if(pattern.uses>=2 && now-pattern.lastAt<=120) {
    const auto edges=ctx.observedSmokeEdges.find(node); if(edges==ctx.observedSmokeEdges.end()) continue;
    for(const auto& edge:edges->second) if(definition_->nodes.count(edge) && ctx.reachableNodes.count(edge))
      offers.push_back({StrategyOp::BeginSmokeWatch,node,edge,"",.5+pattern.uses*.15});
  }
  const auto shot=s.hunterRuntime.pressureAt.find(static_cast<int>(HunterPressure::Gunshot));
  const auto blast=s.hunterRuntime.pressureAt.find(static_cast<int>(HunterPressure::Explosion));
  if((shot!=s.hunterRuntime.pressureAt.end() && now-shot->second<=8) || (blast!=s.hunterRuntime.pressureAt.end() && now-blast->second<=8))
    for(const auto& cover:ctx.observedCoverNodes) if(definition_->nodes.count(cover) && cover!=ctx.actualHunterNode && ctx.reachableNodes.count(cover))
      offers.push_back({StrategyOp::BeginRetreat,cover,cover,"",.6});
  return offers;
}
InteractionResult CampaignCore::ExecuteAdaptive(State& s,const StrategyCommand& c,const StrategyContext& ctx) const {
  const InteractionResult denied{Status::MissingRequirement}; auto& r=s.strategy;
  if(c.kind==StrategyOp::CancelPlan) return r.hunterPlan && r.hunterPlan->serial==c.serial && r.hunterPlan->target==c.target ? InteractionResult{Status::Applied,true} : denied;
  if(ctx.actualHunterNode!=c.node) return denied;
  if(c.kind==StrategyOp::NoticeSmoke) {
    const auto cloud=s.combat.smoke.find(c.serial);
    if(!ctx.lineOfSightVerified || cloud==s.combat.smoke.end() || cloud->second.node!=c.target || r.seenSmoke.count(c.serial)) return denied;
    return {Status::Applied,true};
  }
  if(c.kind==StrategyOp::BeginTrap || c.kind==StrategyOp::BeginSmokeWatch || c.kind==StrategyOp::BeginRetreat) {
    const auto offers=HunterPlanOffers(s,ctx);
    const auto match=std::find_if(offers.begin(),offers.end(),[&](const auto& o) {return o.kind==c.kind && o.target==c.target && o.destination==c.destination && o.auxiliary==c.reference;});
    if(match==offers.end()) return denied;
    if(c.kind==StrategyOp::BeginTrap && !ctx.alternativePathVerified) return denied;
    return {Status::Applied,true};
  }
  if(!r.hunterPlan || r.hunterPlan->interrupted || r.hunterPlan->serial!=c.serial || r.hunterPlan->step!=static_cast<unsigned>(c.argument)) return denied;
  const auto& p=*r.hunterPlan; const auto [target,node]=Step(p,*definition_);
  if(c.target!=target || c.node!=node) return denied;
  if(c.kind==StrategyOp::ArrivePlan) {
    if(p.arrivedAt>=0 || !ctx.movementCompleted || !ctx.reachableNodes.count(node) || !ctx.reactionCueVerified) return denied;
    return {Status::Applied,true};
  }
  const double delay=p.kind==StrategyOp::BeginSmokeWatch || (p.kind==StrategyOp::BeginTrap && p.step==2) ? HunterProfile(s.routes.DifficultyLevel()).watchSeconds : HunterProfile(s.routes.DifficultyLevel()).reactionSeconds;
  if(p.arrivedAt<0 || c.atSeconds<p.arrivedAt+delay || !ctx.continuityVerified || !ctx.actionCompleted || !ctx.lineOfSightVerified) return denied;
  InteractionResult result{Status::Applied,true};
  if(p.kind==StrategyOp::BeginTrap && p.step<2) {
    const auto* object=s.world.Find(target); const auto seen=s.evidence.CaptureSnapshot().lastSeen;
    if(!ctx.alternativePathVerified || !ctx.safeToAlter.count(target) || !object || !object->present || !seen.count(target) || seen.at(target)!=object->stateId) return denied;
    world::WorldCore devices(s.world,definition_->doors,definition_->cameras,definition_->hides);
    if(p.step==0) { result.signal=devices.SetCircuit(target,false,c.atSeconds); if(!result.signal) return denied; }
    else if(!devices.CloseDoor(target,c.atSeconds)) return denied;
    const auto changed=s.world.Find(target); s.evidence.Inspect(target,changed->stateId,changed->positionMeters,c.atSeconds);
  }
  return result;
}
bool CampaignCore::ReplayAdaptive(StrategyRuntimeSnapshot& r,const StrategyCommand& c,std::uint64_t order,std::uint64_t harm) const {
  if(c.kind==StrategyOp::NoticeSmoke) {
    if(!r.seenSmoke.emplace(c.serial,c.target).second) return false;
    auto& p=r.smokePatterns[c.target]; if(c.atSeconds-p.lastAt>120) p.uses=0;
    p.uses=std::min(8u,p.uses+1); p.lastAt=c.atSeconds;
  } else if(c.kind==StrategyOp::BeginTrap || c.kind==StrategyOp::BeginSmokeWatch || c.kind==StrategyOp::BeginRetreat) {
    if(r.hunterPlan || r.question || (r.lastPlanAt>=0 && c.atSeconds-r.lastPlanAt<HunterProfile(state_->routes.DifficultyLevel()).ambushCooldown)) return false;
    if(c.kind==StrategyOp::BeginSmokeWatch && (!r.smokePatterns.count(c.target) || r.smokePatterns.at(c.target).uses<2 || c.atSeconds-r.smokePatterns.at(c.target).lastAt>120)) return false;
    r.hunterPlan=HunterPlan{order,harm,c.kind,c.target,c.destination,c.reference,c.eventId,0,c.atSeconds,-1,false};
  } else {
    if(!r.hunterPlan || r.hunterPlan->serial!=c.serial) return false;
    auto& p=*r.hunterPlan;
    if(c.kind==StrategyOp::CancelPlan) { if(c.target!=p.target) return false; r.hunterPlan.reset(); r.lastPlanAt=c.atSeconds; }
    else {
      const auto [target,node]=Step(p,*definition_);
      if(c.argument!=static_cast<int>(p.step) || target!=c.target || node!=c.node) return false;
      if(c.kind==StrategyOp::ArrivePlan) { if(p.arrivedAt>=0) return false; p.arrivedAt=c.atSeconds; }
      else {
        const double delay=p.kind==StrategyOp::BeginSmokeWatch || (p.kind==StrategyOp::BeginTrap && p.step==2) ? HunterProfile(state_->routes.DifficultyLevel()).watchSeconds : HunterProfile(state_->routes.DifficultyLevel()).reactionSeconds;
        if(p.arrivedAt<0 || c.atSeconds<p.arrivedAt+delay || p.harm!=harm) return false;
        if(++p.step==(p.kind==StrategyOp::BeginTrap ? 3u : 1u)) { r.hunterPlan.reset(); r.lastPlanAt=c.atSeconds; }
        else p.arrivedAt=-1;
      }
    }
  }
  r.serial=order; return true;
}
} // namespace thug::campaign
