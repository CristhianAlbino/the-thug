#include "CampaignState.h"
#include "StrategyIO.h"
#include "CompanionIO.h"
#include "RescueIO.h"
#include "HunterIO.h"
#include <algorithm>

namespace thug::campaign {
bool CampaignCore::ReplayStrategy(StrategyRuntimeSnapshot& r,const StrategyCommand& c,std::uint64_t order,
    bool accepted,std::uint64_t actorHarm,std::uint64_t peerHarm) const {
  if(order!=r.serial+1 || (!accepted && c.kind!=StrategyOp::BeginJointUse)) return false;
  if(!accepted) { r.serial=order; return true; }
  if(c.kind>=StrategyOp::NoticeSmoke) return ReplayAdaptive(r,c,order,actorHarm);
  switch(c.kind) {
    case StrategyOp::DropItem:
      if(!r.drops.emplace(c.eventId,DroppedStack{c.actor,c.item,c.node,c.argument,c.position,{c.actor}}).second) return false;
      r.knownDrops[c.actor][c.eventId]={c.item,c.node,c.argument,c.position,c.atSeconds};
      break;
    case StrategyOp::NoticeDrop: case StrategyOp::TakeDrop: {
      const auto d=r.drops.find(c.target);
      if(d==r.drops.end() || d->second.node!=c.node || d->second.quantity<=0) return false;
      if(c.kind==StrategyOp::NoticeDrop) {
        if(r.knownDrops.count(c.actor) && r.knownDrops.at(c.actor).count(c.target) &&
            r.knownDrops.at(c.actor).at(c.target).atSeconds==c.atSeconds && r.knownDrops.at(c.actor).at(c.target).quantity==d->second.quantity) return false;
        d->second.knownTo.insert(c.actor);
      } else {
        if(!d->second.knownTo.count(c.actor) || c.argument>d->second.quantity) return false;
        d->second.quantity-=c.argument;
      }
      r.knownDrops[c.actor][c.target]={d->second.item,d->second.node,d->second.quantity,d->second.position,c.atSeconds};
      break;
    }
    case StrategyOp::NoticeMissingDrop: {
      const auto d=r.drops.find(c.target);
      if(d==r.drops.end() || d->second.quantity || !d->second.knownTo.count(c.actor) || d->second.node!=c.node || r.knownDrops[c.actor][c.target].quantity==0) return false;
      r.knownDrops[c.actor][c.target].quantity=0; r.knownDrops[c.actor][c.target].atSeconds=c.atSeconds; break;
    }
    case StrategyOp::ObserveArea:
      r.observations[c.actor][c.destination]={c.eventId,c.destination,c.atSeconds,c.argument!=0}; break;
    case StrategyOp::ReportThreat: {
      double at=c.atSeconds;
      if(c.argument && c.destination!=c.node && (!r.observations.count(c.actor) || !r.observations.at(c.actor).count(c.destination))) return false;
      if(!c.argument) {
        const auto a=r.observations.find(c.actor);
        if(a==r.observations.end() || !a->second.count(c.destination)) return false;
        const auto& o=a->second.at(c.destination);
        if(o.eventId!=c.reference || !o.hunterPresent) return false;
        at=o.atSeconds;
      }
      if(!r.reports.emplace(c.eventId,InformationReport{c.actor,c.target,c.destination,"",at,true,c.argument!=0,false,false}).second) return false;
      break;
    }
    case StrategyOp::CheckReport: {
      const auto p=r.reports.find(c.target); const auto a=r.observations.find(c.actor);
      if(p==r.reports.end() || p->second.recipient!=c.actor || p->second.checked || a==r.observations.end() ||
          !a->second.count(p->second.node)) return false;
      const auto& o=a->second.at(p->second.node);
      if(o.eventId!=c.reference || o.atSeconds<p->second.atSeconds || o.atSeconds>p->second.atSeconds+2 || c.atSeconds>o.atSeconds+2) return false;
      p->second.checked=true; p->second.contradicted=!o.hunterPresent; break;
    }
    case StrategyOp::BeginQuestion:
      if(r.question || r.joint) return false;
      r.question=StrategyTask{order,actorHarm,peerHarm,c.kind,c.actor,c.target,std::to_string(c.serial),c.node,c.eventId,c.atSeconds,c.atSeconds+QuestionSeconds,false}; break;
    case StrategyOp::AnswerQuestion:
      if(!r.question || r.question->peer!=c.actor || r.question->actor!=c.target || r.question->serial!=c.serial ||
          r.question->node!=c.node || c.atSeconds<r.question->readyAt || r.question->actorHarm!=peerHarm || r.question->peerHarm!=actorHarm) return false;
      if(c.argument) r.reports.emplace(c.eventId,InformationReport{c.actor,"hunter",detail::ExitNode(c.item),c.item,c.atSeconds,false,c.argument==2,false,false});
      r.question.reset(); break;
    case StrategyOp::CancelQuestion:
      if(!r.question || r.question->actor!=c.actor || r.question->peer!=c.target || r.question->serial!=c.serial || r.question->node!=c.node) return false;
      r.question.reset(); break;
    case StrategyOp::BeginJointUse:
      if(r.joint || r.question) return false;
      r.joint=StrategyTask{order,actorHarm,peerHarm,c.kind,c.actor,c.target,c.reference,c.node,c.eventId,c.atSeconds,c.atSeconds+JointSeconds,false}; break;
    case StrategyOp::CompleteJointUse: case StrategyOp::CancelJointUse:
      if(!r.joint || r.joint->actor!=c.actor || r.joint->peer!=c.target || r.joint->node!=c.node || r.joint->serial!=c.serial) return false;
      if(c.kind==StrategyOp::CompleteJointUse && (c.atSeconds<r.joint->readyAt || r.joint->actorHarm!=actorHarm || r.joint->peerHarm!=peerHarm)) return false;
      r.joint.reset(); break;
    default: return false;
  }
  r.serial=order; return true;
}
void CampaignCore::WriteStrategy(std::ostream& out,const State& s) const {
  out << "STRATEGY " << s.strategy.serial << ' ' << detail::StrategyFingerprint(s.strategy) << '\n';
}
bool CampaignCore::ReadStrategy(std::istream& in,State& s) const {
  std::string tag,hash; std::uint64_t serial;
  if(!(in>>tag>>serial>>hash) || tag!="STRATEGY" || hash.size()!=16 || serial>detail::MaxReceipts) return false;
  s.strategy={}; s.strategy.serial=serial; s.loadedStrategyFingerprint=hash; return true;
}
bool CampaignCore::RestoreStrategy(State& s) const {
  struct Entry { StrategyCommand command; bool accepted; std::uint64_t actorHarm,peerHarm; };
  std::map<std::uint64_t,Entry> ordered;
  const auto expectedSerial=s.strategy.serial;
  for(const auto& [id,signature]:s.receipts) if(signature.rfind("Q ",0)==0) {
    Entry e; std::uint64_t order; e.command.eventId=id;
    if(!detail::ParseStrategy(signature,e.command,order,e.accepted,e.actorHarm,e.peerHarm) || !StrategyStaticValid(e.command) ||
        !order || e.command.atSeconds>s.world.NowSeconds() || !ordered.emplace(order,e).second) return false;
    const double after=std::nextafter(e.command.atSeconds,std::numeric_limits<double>::infinity());
    const auto validHarm=[&](const std::string& actor,std::uint64_t count) {
      return count>=RescueHarmCount(s,actor,e.command.atSeconds) && count<=RescueHarmCount(s,actor,after);
    };
    if(!validHarm(e.command.actor,e.actorHarm) || !validHarm(e.command.target,e.peerHarm)) return false;
  }
  if(ordered.size()!=expectedSerial) return false;
  StrategyRuntimeSnapshot rebuilt; double previous=0;
  for(const auto& [order,e]:ordered) {
    const auto& c=e.command;
    if(c.kind==StrategyOp::BeginQuestion) {
      if(c.serial>s.rescue.serial) return false;
      std::map<std::uint64_t,RescueCommand> custody;
      for(const auto& [id,sig]:s.receipts) if(sig.rfind("R ",0)==0) {
        (void)id; RescueCommand event; std::uint64_t serial,a,p; bool seen;
        if(!detail::ParseRescue(sig,event,serial,seen,a,p)) return false;
        if(serial<=c.serial && event.target==c.target) { if(event.atSeconds>c.atSeconds) return false; custody.emplace(serial,event); }
      }
      CapturePhase phase=CapturePhase::Released; bool release=false;
      for(const auto& [serial,event]:custody) {
        (void)serial;
        if(event.kind==RescueOp::BeginContainment) phase=CapturePhase::Restraining;
        if(event.kind==RescueOp::FinishContainment) phase=CapturePhase::Contained;
        if(event.kind==RescueOp::CancelContainment || event.kind==RescueOp::CompleteRelease) phase=CapturePhase::Released;
        if(event.kind==RescueOp::FinalizeCapture) phase=CapturePhase::Finalized;
        if(event.kind==RescueOp::BeginRelease || event.kind==RescueOp::BeginSelfRelease) release=true;
        if(event.kind==RescueOp::CompleteRelease || event.kind==RescueOp::CancelRelease || event.kind==RescueOp::FinalizeCapture) release=false;
      }
      if(phase!=CapturePhase::Contained || release) return false;
    }
    if(c.kind==StrategyOp::AnswerQuestion && c.argument==1) {
      bool known=false;
      for(const auto& [id,sig]:s.receipts) if(sig.rfind("C ",0)==0) {
        (void)id; CompanionCommand route;
        if(detail::ParseCompanion(sig,route) && route.kind==CompanionOp::ObserveRoute && route.actor==c.actor && route.target==c.item &&
            rebuilt.question && route.atSeconds<=rebuilt.question->startedAt) known=true;
      }
      if(!known) return false;
    }
    if(s.routes.DifficultyLevel()==rules::Difficulty::Easy && ((c.kind==StrategyOp::AnswerQuestion && c.argument==2) ||
        (c.kind==StrategyOp::ReportThreat && c.argument && c.actor!="joe"))) return false;
    if(c.kind==StrategyOp::NoticeSmoke) {
      bool source=false;
      for(const auto& [id,sig]:s.receipts) if(sig.rfind("W ",0)==0) {
        (void)id; CombatCommand cloud; std::uint64_t n,created;
        if(detail::ParseCombat(sig,cloud,n,created) && cloud.kind==CombatOp::Detonate && cloud.target=="smoke" && cloud.serial==c.serial &&
            cloud.node==c.target && cloud.atSeconds<=c.atSeconds && c.atSeconds<cloud.atSeconds+CombatProfile(s.routes.DifficultyLevel()).smokeSeconds) source=true;
      }
      if(!source) return false;
    }
    if(c.kind==StrategyOp::BeginTrap) {
      std::set<double> passages;
      for(const auto& [id,sig]:s.receipts) if(sig.rfind("H ",0)==0) {
        (void)id; HunterCommand pass; std::optional<HunterTask> plan;
        if(detail::ParseHunter(sig,pass,plan) && pass.kind==HunterOp::NoticePassage && pass.target==c.target && pass.atSeconds<=c.atSeconds && c.atSeconds-pass.atSeconds<=120) passages.insert(pass.atSeconds);
      }
      if(passages.size()<2 || *passages.rbegin()-*passages.begin()<15) return false;
    }
    if(e.command.atSeconds<previous || !ReplayStrategy(rebuilt,e.command,order,e.accepted,e.actorHarm,e.peerHarm)) return false;
    previous=e.command.atSeconds;
  }
  s.strategy=std::move(rebuilt); ReconcileStrategy(s);
  return s.loadedStrategyFingerprint.empty() || s.loadedStrategyFingerprint==detail::StrategyFingerprint(s.strategy);
}
bool CampaignCore::StrategyCoherent(const State& s) const {
  auto expected=std::make_unique<State>(s);
  expected->loadedStrategyFingerprint=detail::StrategyFingerprint(s.strategy);
  if(!RestoreStrategy(*expected) || s.receipts.size()+detail::CombatReservedReceipts(s.combat)+
      detail::SpecialReservedReceipts(s.special)+detail::RescueReservedReceipts(s.rescue)+detail::StrategyReservedReceipts(s.strategy)>detail::MaxReceipts) return false;
  for(const auto* task:{&s.strategy.question,&s.strategy.joint}) if(*task) {
    const auto& t=**task;
    if(t.interrupted) continue;
    for(const auto& actor:{t.actor,t.peer}) if(actor!="hunter") {
      if(actor=="joe") { if(!s.hidingPlace.empty() || s.combat.task || s.special.plant) return false; }
      else { const auto& b=s.bodies.at(actor); if(b.task || !b.hidingPlace.empty() || b.lastNode!=t.node) return false; }
    }
    if(t.kind==StrategyOp::BeginJointUse) {
      const auto u=std::find_if(definition_->uses.begin(),definition_->uses.end(),[&](const auto& v){return v.id==t.target;});
      if(u==definition_->uses.end()) return false;
      const auto& bag=t.peer=="joe" ? s.inventory : s.bodies.at(t.peer).inventory;
      if(!u->item.empty() && bag.Quantity(u->item)<u->quantity) return false;
    } else if(s.hunterRuntime.task) return false;
  }
  return true;
}
} // namespace thug::campaign
