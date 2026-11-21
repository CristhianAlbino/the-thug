#include "CampaignState.h"
#include "StrategyIO.h"
#include "CompanionIO.h"
#include "RescueIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& entries,const std::string& id) {
  for(const auto& e:entries) if(e.id==id) return &e;
  return nullptr;
}
bool Actor(const std::string& a) { return a=="joe" || bool(detail::Person(a)); }
bool At(const StrategyContext& c,const std::string& a,const std::string& n) {
  const auto it=c.actors.find(a); return it!=c.actors.end() && it->second.active && it->second.node==n;
}
bool Active(const save::NarrativeSnapshot& story,const std::string& a) {
  if(a=="joe" || a=="hunter") return true;
  const auto& o=story.outcomes.at(a); return !o.dead && !o.captured && !o.together && !o.escapedAlone;
}
bool Terminal(StrategyOp op) {
  return op==StrategyOp::AnswerQuestion || op==StrategyOp::CancelQuestion ||
    op==StrategyOp::CompleteJointUse || op==StrategyOp::CancelJointUse;
}
}
StrategyRuntimeSnapshot CampaignCore::StrategyRuntime() const { return state_->strategy; }
std::map<std::string,KnownDrop> CampaignCore::KnownDrops(const std::string& actor) const {
  std::map<std::string,KnownDrop> result;
  if(!Actor(actor)) return result;
  const auto known=state_->strategy.knownDrops.find(actor);
  if(known!=state_->strategy.knownDrops.end()) for(const auto& [id,d]:known->second) if(d.quantity) result.emplace(id,d);
  return result;
}
std::vector<KnownReport> CampaignCore::KnownReports(const std::string& actor) const {
  std::vector<KnownReport> result;
  if(!Actor(actor) && actor!="hunter") return result;
  for(const auto& [id,r]:state_->strategy.reports) if(r.recipient==actor)
    result.push_back({id,r.source,r.node,r.route,r.atSeconds,r.hunterPresent,r.checked,r.contradicted,
      r.route.empty() && Now()-r.atSeconds>ReportFreshSeconds});
  return result;
}
bool CampaignCore::StrategyBusy(const State& s,const std::string& actor) const {
  for(const auto* task:{&s.strategy.question,&s.strategy.joint})
    if(*task && ((**task).actor==actor || (**task).peer==actor)) return true;
  return false;
}
std::vector<AnswerKind> CampaignCore::QuestionOptions() const {
  std::vector<AnswerKind> options;
  if(Paused() || Ending()!=rules::Ending::None || !state_->strategy.question || state_->strategy.question->interrupted) return options;
  const auto& t=*state_->strategy.question;
  const auto& p=state_->Person(*detail::Person(t.peer)); const auto& traits=p.CurrentTraits();
  options.push_back(AnswerKind::Refuse);
  if(!p.KnownRoutes().empty() && (traits.fear+traits.selfPreservation>=120 || traits.loyalty<40)) options.push_back(AnswerKind::RevealRoute);
  if(state_->routes.DifficultyLevel()!=rules::Difficulty::Easy && traits.selfPreservation>=50) options.push_back(AnswerKind::Mislead);
  return options;
}
bool CampaignCore::StrategyStaticValid(const StrategyCommand& c) const {
  if(c.kind>=StrategyOp::NoticeSmoke) return AdaptiveStaticValid(c);
  if(!detail::Token(c.eventId) || !definition_->nodes.count(c.node) || !std::isfinite(c.atSeconds) || c.atSeconds<0 ||
      c.kind<StrategyOp::DropItem || c.kind>StrategyOp::CancelJointUse || !detail::CombatFinite(c.position)) return false;
  if(c.kind!=StrategyOp::DropItem && (c.position.x || c.position.y || c.position.z)) return false;
  if((Terminal(c.kind) || c.kind==StrategyOp::BeginQuestion) ? c.serial==0 : c.serial!=0) return false;
  if(c.kind!=StrategyOp::ReportThreat && c.kind!=StrategyOp::ObserveArea && !c.destination.empty()) return false;
  if(c.kind!=StrategyOp::ReportThreat && c.kind!=StrategyOp::CheckReport && c.kind!=StrategyOp::BeginJointUse && !c.reference.empty()) return false;
  if(c.kind!=StrategyOp::DropItem && c.kind!=StrategyOp::AnswerQuestion && !c.item.empty()) return false;
  switch(c.kind) {
    case StrategyOp::DropItem: return Actor(c.actor) && c.target.empty() && c.argument>0 && c.argument<=100 && detail::Token(c.item);
    case StrategyOp::NoticeDrop: return Actor(c.actor) && detail::Token(c.target) && !c.argument;
    case StrategyOp::NoticeMissingDrop: return Actor(c.actor) && detail::Token(c.target) && !c.argument;
    case StrategyOp::TakeDrop: return Actor(c.actor) && detail::Token(c.target) && c.argument>0 && c.argument<=100;
    case StrategyOp::ObserveArea: return Actor(c.actor) && c.target.empty() && definition_->nodes.count(c.destination) && c.argument>=0 && c.argument<=1;
    case StrategyOp::ReportThreat: return Actor(c.actor) && Actor(c.target) && c.actor!=c.target && definition_->nodes.count(c.destination) && c.argument>=0 && c.argument<=1 &&
      (c.argument ? c.reference.empty() : detail::Token(c.reference));
    case StrategyOp::CheckReport: return Actor(c.actor) && detail::Token(c.target) && detail::Token(c.reference) && !c.argument;
    case StrategyOp::BeginQuestion: case StrategyOp::CancelQuestion: return c.actor=="hunter" && detail::Person(c.target) && !c.argument;
    case StrategyOp::AnswerQuestion: return detail::Person(c.actor) && c.target=="hunter" && c.argument>=0 && c.argument<=2 &&
      (c.argument==0 ? c.item.empty() : bool(detail::Route(c.item)));
    case StrategyOp::BeginJointUse: {
      const auto* use=Find(definition_->uses,c.reference);
      return Actor(c.actor) && Actor(c.target) && c.actor!=c.target && !c.argument && use && use->node==c.node && use->id!="safe_code_accepted";
    }
    case StrategyOp::CompleteJointUse: case StrategyOp::CancelJointUse: return Actor(c.actor) && Actor(c.target) && c.actor!=c.target && !c.argument;
    default: return false;
  }
  return false;
}
void CampaignCore::ReconcileStrategy(State& s) const {
  ReconcileAdaptive(s);
  const auto story=s.narrative.CaptureSnapshot();
  for(auto* task:{&s.strategy.question,&s.strategy.joint}) if(*task) {
    auto& t=**task;
    const auto hurt=[&](const std::string& a) {
      if(a=="hunter") return s.world.NowSeconds()<s.combat.hunterBlockedUntil;
      const auto h=a=="joe" ? s.survival.CurrentEffects() : s.bodies.at(a).survival.CurrentEffects();
      return !Active(story,a) || h.dead || h.collapsed;
    };
    t.interrupted=t.interrupted || s.routes.Result()!=rules::Ending::None || hurt(t.actor) || hurt(t.peer) ||
      RescueHarmCount(s,t.actor)!=t.actorHarm || RescueHarmCount(s,t.peer)!=t.peerHarm;
    if(t.kind==StrategyOp::BeginQuestion) {
      const auto captured=s.rescue.captures.find(t.peer);
      t.interrupted=t.interrupted || captured==s.rescue.captures.end() || captured->second.phase!=CapturePhase::Contained ||
        bool(captured->second.release);
      for(const auto& [id,signature]:s.receipts) if(signature.rfind("R ",0)==0) {
        (void)id; RescueCommand command; std::uint64_t order,a,p; bool seen;
        if(detail::ParseRescue(signature,command,order,seen,a,p) && order>std::stoull(t.target) &&
            command.target==t.peer && (command.kind==RescueOp::BeginRelease || command.kind==RescueOp::BeginSelfRelease)) t.interrupted=true;
      }
    } else {
      for(const auto& [victim,c]:s.rescue.captures) if(victim==t.actor || victim==t.peer)
        t.interrupted=t.interrupted || c.phase==CapturePhase::Restraining || c.phase==CapturePhase::Contained;
    }
  }
}
InteractionResult CampaignCore::ApplyStrategy(const StrategyCommand& c,const StrategyContext& ctx) {
  if(!StrategyStaticValid(c)) return {Status::InvalidAction};
  const auto old=state_->receipts.find(c.eventId);
  if(old!=state_->receipts.end()) {
    StrategyCommand saved; std::uint64_t order,a,p; bool accepted;
    return {detail::ParseStrategy(old->second,saved,order,accepted,a,p) &&
      old->second==detail::Signature(c,order,accepted,a,p) ? Status::AlreadyApplied : Status::Conflict};
  }
  if(Paused()) return {Status::Paused};
  const bool cancel=c.kind==StrategyOp::CancelQuestion || c.kind==StrategyOp::CancelJointUse || c.kind==StrategyOp::CancelPlan;
  if(Ending()!=rules::Ending::None && !cancel) return {Status::Finished};
  if(c.atSeconds!=Now()) return {Status::InvalidTime};
  if(!c.physicallyVerified || ctx.actors.size()>4 ||
      (!ctx.actualHunterNode.empty() && !definition_->nodes.count(ctx.actualHunterNode))) return {Status::InvalidAction};
  for(const auto& [actor,a]:ctx.actors) if(!Actor(actor) || !definition_->nodes.count(a.node)) return {Status::InvalidAction};
  if(state_->special.phase==SpecialPhase::Collapsing ||
      (state_->special.phase==SpecialPhase::Confronting && Now()>=state_->special.boundaryAt)) return {Status::AwaitingPhysics};
  if(state_->strategy.serial==std::numeric_limits<std::uint64_t>::max()) return {Status::LogFull};
  auto next=std::make_unique<State>(*state_);
  ReconcileStrategy(*next);
  const auto result=ExecuteStrategy(*next,c,ctx);
  if(result.status!=Status::Applied) return result;
  const auto actorHarm=RescueHarmCount(*next,c.actor);
  const auto peerHarm=(Actor(c.target) || c.target=="hunter") ? RescueHarmCount(*next,c.target) : 0;
  if(!ReplayStrategy(next->strategy,c,next->strategy.serial+1,result.accepted,actorHarm,peerHarm)) return {Status::Conflict};
  if(next->receipts.size()+1+detail::CombatReservedReceipts(next->combat)+detail::SpecialReservedReceipts(next->special)+
      detail::RescueReservedReceipts(next->rescue)+detail::StrategyReservedReceipts(next->strategy)>detail::MaxReceipts) return {Status::LogFull};
  next->receipts.emplace(c.eventId,detail::Signature(c,next->strategy.serial,result.accepted,actorHarm,peerHarm));
  next->SyncEnding(); ReconcileCombat(*next);
  state_.swap(next); return result;
}
InteractionResult CampaignCore::ExecuteStrategy(State& s,const StrategyCommand& c,const StrategyContext& ctx) const {
  if(c.kind>=StrategyOp::NoticeSmoke) return ExecuteAdaptive(s,c,ctx);
  const InteractionResult denied{Status::MissingRequirement};
  const auto story=s.narrative.CaptureSnapshot();
  auto& r=s.strategy;
  const bool cancel=c.kind==StrategyOp::CancelQuestion || c.kind==StrategyOp::CancelJointUse || c.kind==StrategyOp::CancelPlan;
  if(cancel) {
    const auto& task=c.kind==StrategyOp::CancelQuestion ? r.question : r.joint;
    return task && task->serial==c.serial && task->actor==c.actor && task->peer==c.target && task->node==c.node ? InteractionResult{Status::Applied,true} : denied;
  }
  const bool answer=c.kind==StrategyOp::AnswerQuestion;
  const bool complete=c.kind==StrategyOp::CompleteJointUse;
  if(c.actor=="hunter") { if(ctx.actualHunterNode!=c.node) return denied; }
  else {
    const auto& h=c.actor=="joe" ? s.survival : s.bodies.at(c.actor).survival;
    if(!At(ctx,c.actor,c.node) || !Active(story,c.actor) || h.CurrentEffects().dead || h.CurrentEffects().collapsed ||
        !RescueLocation(s,c.actor,c.node)) return denied;
    if(!answer && !complete && (RescueBusy(s,c.actor) || WeaponBusy(s,c.actor))) return denied;
    if(c.actor=="joe" ? !s.hidingPlace.empty() || bool(s.combat.task) || bool(s.special.plant) :
        !s.bodies.at(c.actor).hidingPlace.empty() || bool(s.bodies.at(c.actor).task)) return denied;
    if(s.special.phase==SpecialPhase::Confronting || s.special.phase==SpecialPhase::Opportunity) return denied;
  }
  auto& bag=c.actor=="hunter" || c.actor=="joe" ? s.inventory : s.bodies.at(c.actor).inventory;
  const auto syncAmmo=[&](const std::string& actor) {
    if(actor=="joe") return true;
    auto p=s.Person(*detail::Person(actor)).CaptureSnapshot(); int count=0;
    for(const auto& [id,n]:s.bodies.at(actor).inventory.Contents()) if(s.bodies.at(actor).inventory.IsCategory(id,survival::ItemCategory::Ammunition)) count+=n;
    p.traits.ammunition=count; return s.Person(*detail::Person(actor)).RestoreSnapshot(p);
  };
  switch(c.kind) {
    case StrategyOp::DropItem:
      if(!ctx.actionCompleted || (c.actor=="joe" && !CombatItemAvailable(s,c.item,c.argument)) ||
          !bag.Remove(c.item,c.argument,survival::RemovalReason::Drop) || !syncAmmo(c.actor)) return denied;
      break;
    case StrategyOp::NoticeDrop: case StrategyOp::TakeDrop: {
      const auto d=r.drops.find(c.target);
      if(d==r.drops.end() || d->second.node!=c.node || !d->second.quantity || !ctx.actionCompleted) return denied;
      if(c.kind==StrategyOp::NoticeDrop && r.knownDrops.count(c.actor) && r.knownDrops.at(c.actor).count(c.target)) {
        const auto& known=r.knownDrops.at(c.actor).at(c.target);
        if(known.atSeconds==c.atSeconds && known.quantity==d->second.quantity) return denied;
      }
      if(c.kind==StrategyOp::TakeDrop && (!d->second.knownTo.count(c.actor) || c.argument>d->second.quantity ||
          !bag.Add(d->second.item,c.argument) || !syncAmmo(c.actor))) return denied;
      break;
    }
    case StrategyOp::NoticeMissingDrop: {
      const auto d=r.drops.find(c.target);
      if(d==r.drops.end() || d->second.node!=c.node || d->second.quantity || !d->second.knownTo.count(c.actor) ||
          !ctx.actionCompleted || r.knownDrops.at(c.actor).at(c.target).quantity==0) return denied;
      break;
    }
    case StrategyOp::ObserveArea:
      if(!ctx.areaObserved || (c.argument ? ctx.actualHunterNode!=c.destination : ctx.actualHunterNode==c.destination)) return denied;
      break;
    case StrategyOp::ReportThreat:
      if(!ctx.speechPerceived || !At(ctx,c.target,c.node) || !Active(story,c.target) ||
          (c.argument && c.actor!="joe" && s.routes.DifficultyLevel()==rules::Difficulty::Easy)) return denied;
      if(c.argument && c.actor!="joe") {
        const auto& t=s.Person(*detail::Person(c.actor)).CurrentTraits();
        if(t.trust>=50 && t.loyalty>=35 && t.selfPreservation<65) return denied;
      }
      if(c.argument && c.destination!=c.node && (!r.observations.count(c.actor) || !r.observations.at(c.actor).count(c.destination))) return denied;
      break;
    case StrategyOp::CheckReport: {
      const auto p=r.reports.find(c.target);
      if(p==r.reports.end() || p->second.recipient!=c.actor || p->second.checked) return denied;
      const auto o=r.observations.find(c.actor);
      if(o==r.observations.end() || !o->second.count(p->second.node)) return denied;
      const auto& observed=o->second.at(p->second.node);
      if(observed.eventId!=c.reference || observed.atSeconds<p->second.atSeconds ||
          observed.atSeconds>p->second.atSeconds+2 || c.atSeconds>observed.atSeconds+2) return denied;
      if(p->second.deliberateLie && !observed.hunterPresent) {
        const auto person=detail::Person(c.actor);
        if(person && p->second.source=="joe") {
          companion::Influence delta; delta.trust=-15; delta.loyalty=-5;
          if(!s.Person(*person).ApplyInfluence(delta,c.eventId,c.atSeconds)) return denied;
        }
      }
      break;
    }
    case StrategyOp::BeginQuestion: {
      const auto captive=s.rescue.captures.find(c.target);
      if(r.question || r.joint || r.hunterPlan || captive==s.rescue.captures.end() || captive->second.phase!=CapturePhase::Contained ||
          c.serial!=s.rescue.serial || s.special.phase==SpecialPhase::Confronting || s.special.phase==SpecialPhase::Opportunity ||
          captive->second.release || !Active(story,c.target) || s.bodies.at(c.target).survival.CurrentEffects().collapsed ||
          !At(ctx,c.target,c.node) || captive->second.node!=c.node || !ctx.contactVerified || !ctx.speechPerceived ||
          c.atSeconds<s.combat.hunterBlockedUntil || s.hunterRuntime.task) return denied;
      break;
    }
    case StrategyOp::AnswerQuestion: {
      if(!r.question || r.question->peer!=c.actor || r.question->serial!=c.serial || r.question->node!=c.node ||
          r.question->interrupted || c.atSeconds<r.question->readyAt || ctx.actualHunterNode!=c.node ||
          !ctx.contactVerified || !ctx.continuityVerified || !ctx.speechPerceived || !ctx.actionCompleted) return denied;
      const auto& p=s.Person(*detail::Person(c.actor)); const auto& t=p.CurrentTraits();
      if(c.argument==1 && (!p.KnownRoutes().count(c.item) || (t.fear+t.selfPreservation<120 && t.loyalty>=40))) return denied;
      if(c.argument==2 && (s.routes.DifficultyLevel()==rules::Difficulty::Easy || t.selfPreservation<50)) return denied;
      break;
    }
    case StrategyOp::BeginJointUse: {
      if(r.joint || r.question || RescueBusy(s,c.target) || WeaponBusy(s,c.target) || !At(ctx,c.target,c.node) || !Active(story,c.target) || !ctx.speechPerceived) return denied;
      const auto& h=c.target=="joe" ? s.survival : s.bodies.at(c.target).survival;
      if(h.CurrentEffects().collapsed || h.CurrentEffects().dead) return denied;
      for(const auto& actor:{c.actor,c.target}) if(actor!="joe") {
        const auto& b=s.bodies.at(actor); const auto& t=s.Person(*detail::Person(actor)).CurrentTraits();
        if(b.task || !b.hidingPlace.empty()) return denied;
        if(t.trust<35 || t.loyalty<30 || t.fear+t.stress>=180) return {Status::Applied,false};
      }
      if(c.target=="joe" && (s.combat.task || s.special.plant || !s.hidingPlace.empty())) return denied;
      const auto& u=*Find(definition_->uses,c.reference); const auto* o=s.world.Find(u.object);
      if(!o || !o->present || o->stateId!=u.before) return denied;
      auto& supply=c.target=="joe" ? s.inventory : s.bodies.at(c.target).inventory;
      if(!u.item.empty() && (supply.Quantity(u.item)<u.quantity || (c.target=="joe" && !CombatItemAvailable(s,u.item,u.quantity)))) return denied;
      break;
    }
    case StrategyOp::CompleteJointUse: {
      if(!r.joint || r.joint->serial!=c.serial || r.joint->actor!=c.actor || r.joint->peer!=c.target ||
          r.joint->node!=c.node || r.joint->interrupted || c.atSeconds<r.joint->readyAt || !At(ctx,c.target,c.node) ||
          !ctx.continuityVerified || !ctx.actionCompleted) return denied;
      const auto& u=*Find(definition_->uses,r.joint->target); auto object=*s.world.Find(u.object);
      if(!object.present || object.stateId!=u.before) return denied;
      for(const auto& requirement:u.requirements) {
        const auto* o=s.world.Find(requirement.id); if(!o || !o->present || o->stateId!=requirement.state) return denied;
      }
      auto& supply=c.target=="joe" ? s.inventory : s.bodies.at(c.target).inventory;
      if(!u.item.empty() && supply.Quantity(u.item)<u.quantity) return denied;
      if(u.consume && !supply.Remove(u.item,u.quantity,survival::RemovalReason::Consume)) return denied;
      if(u.runEvent && !s.routes.Apply(*u.runEvent,c.atSeconds)) return denied;
      object.stateId=u.after;
      if(!s.world.SetObject(u.object,object,c.atSeconds) || !syncAmmo(c.target)) return denied;
      break;
    }
    default: return denied;
  }
  if(c.actor!="joe" && c.actor!="hunter") s.bodies.at(c.actor).lastNode=c.node;
  if(c.kind==StrategyOp::BeginJointUse && c.target!="joe") s.bodies.at(c.target).lastNode=c.node;
  return {Status::Applied,true};
}
} // namespace thug::campaign
