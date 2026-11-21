#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "StrategyIO.h"
#include <iostream>
#include <stdexcept>
using namespace thug;
using namespace thug::campaign;
namespace {
int checks=0,sequence=0;
void Check(bool ok,const char* why) { ++checks; if(!ok) throw std::runtime_error(why); }
std::string Id() { return "extension-test-"+std::to_string(++sequence); }
std::string Bytes(const CampaignCore& c) { std::ostringstream out; Check(c.Save(out)==save::Result::Ok,"Extension snapshot coherent"); return out.str(); }
void Resume(CampaignCore& c) {
  const auto bytes=Bytes(c); CampaignCore copy(c.Narrative().difficulty,991); std::istringstream in(bytes);
  Check(copy.Load(in)==save::Result::Ok && Bytes(copy)==bytes,"Save/load preserves resources, adaptation and exact stages"); c=std::move(copy);
}
void Tick(CampaignCore& c,double dt) { Check(c.Tick(dt,survival::Activity::Rest).status==Status::Applied,"Actual campaign delta advances"); }
CombatContext CombatCtx() {
  CombatContext ctx; ctx.actualJoeNode="F4_START"; ctx.physicallyVerified=ctx.animationCompleted=ctx.collisionVerified=ctx.hunterPerceived=true;
  for(const auto& actor:{"harley","bartholomeu","jack"}) ctx.actualActorNodes[actor]=ctx.actualJoeNode;
  return ctx;
}
CombatCommand W(const CampaignCore& c,CombatOp kind,const std::string& target="rifle",const std::string& actor="joe",std::uint64_t serial=0) {
  CombatCommand cmd; cmd.eventId=Id(); cmd.actor=actor; cmd.kind=kind; cmd.target=target; cmd.node="F4_START"; cmd.atSeconds=c.Now(); cmd.serial=serial;
  if(kind==CombatOp::Fire) cmd.vector={1,0,0};
  return cmd;
}
CombatResult ApplyW(CampaignCore& c,const CombatCommand& cmd,const CombatContext& ctx) {
  const auto result=c.ApplyCombat(cmd,ctx); if(result.status!=Status::Applied) std::cerr<<"Combat kind "<<static_cast<int>(cmd.kind)<<" actor "<<cmd.actor<<" status "<<static_cast<int>(result.status)<<'\n'; Check(result.status==Status::Applied,"Actor combat physically commits"); const auto bytes=Bytes(c);
  Check(c.ApplyCombat(cmd,ctx).status==Status::AlreadyApplied && Bytes(c)==bytes,"Actor command cannot replay resource use"); return result;
}
StrategyContext Ctx(const std::string& node="F4_HALL_E") {
  StrategyContext c; c.actualHunterNode=node;
  c.actionCompleted=c.continuityVerified=c.movementCompleted=c.lineOfSightVerified=c.reactionCueVerified=c.alternativePathVerified=true;
  c.reachableNodes=data::Hospital().nodes; return c;
}
StrategyCommand Q(const CampaignCore& c,StrategyOp kind,const std::string& target,const std::string& node="F4_HALL_E",std::uint64_t serial=0,int step=0) {
  StrategyCommand cmd; cmd.eventId=Id(); cmd.kind=kind; cmd.actor="hunter"; cmd.target=target; cmd.node=node; cmd.serial=serial; cmd.argument=step; cmd.atSeconds=c.Now(); cmd.physicallyVerified=true; return cmd;
}
void ApplyQ(CampaignCore& c,const StrategyCommand& q,const StrategyContext& ctx) {
  const auto result=c.ApplyStrategy(q,ctx); if(result.status!=Status::Applied) std::cerr<<"Adaptive command "<<static_cast<int>(q.kind)<<" returned "<<static_cast<int>(result.status)<<'\n';
  Check(result.status==Status::Applied && result.accepted,"Adaptive physical step commits"); const auto bytes=Bytes(c);
  Check(c.ApplyStrategy(q,ctx).status==Status::AlreadyApplied && Bytes(c)==bytes,"Adaptive receipt is idempotent");
}
void RejectQ(CampaignCore& c,const StrategyCommand& q,const StrategyContext& ctx) {
  const auto bytes=Bytes(c); Check(c.ApplyStrategy(q,ctx).status==Status::MissingRequirement,"Adaptive action rejects missing physics, opportunity or timing"); Check(Bytes(c)==bytes,"Rejected adaptive action has no world effect");
}
CompanionContext Party() {
  CompanionContext ctx; ctx.actionCompleted=true;
  for(const auto& a:{"joe","harley","bartholomeu","jack"}) ctx.actors.emplace(a,director::Actor{"F4_START",true});
  return ctx;
}
void Share(CampaignCore& c,const std::string& actor,const std::string& item) {
  CompanionCommand a{Id(),CompanionOp::Transfer,"joe",actor,"F4_START",c.Now()}; a.item=item; a.quantity=1; a.physicallyVerified=true;
  Check(c.ApplyCompanion(a,Party()).status==Status::Applied,"Physical inventory transfer commits"); Bytes(c);
}
void ActorWeapons() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive})
    for(const auto& actor:{"harley","bartholomeu","jack"}) {
      CampaignCore c(mode,201); auto ctx=CombatCtx(); Share(c,actor,"rifle"); Share(c,actor,"magazine");
      Check(!c.CompanionCombatOffers(actor,ctx).empty(),"Companion has an executable weapon choice");
      auto task=ApplyW(c,W(c,CombatOp::BeginReload,"rifle",actor),ctx).task; Check(bool(task),"Each actor owns its reload serial"); Resume(c);
      const auto before=Bytes(c); auto drop=StrategyCommand{}; drop.eventId=Id(); drop.actor=actor; drop.kind=StrategyOp::DropItem; drop.item="magazine"; drop.argument=1; drop.node="F4_START"; drop.atSeconds=c.Now(); drop.physicallyVerified=true;
      auto social=Ctx(); social.actors.emplace(actor,director::Actor{"F4_START",true}); social.actionCompleted=true;
      Check(c.ApplyStrategy(drop,social).status==Status::MissingRequirement && Bytes(c)==before,"Another family cannot spend a magazine reserved for reload");
      Tick(c,CombatProfile(mode).reloadSeconds); ApplyW(c,W(c,CombatOp::CompleteReload,"rifle",actor,task->serial),ctx);
      Check(c.CombatRuntime().loadedRounds==0 && c.CombatRuntime().companions.at(actor).loadedRounds==30,"NPC reload loads only its own weapon");
      const auto ready=Bytes(c); auto blind=ctx; blind.hunterPerceived=false;
      Check(c.ApplyCombat(W(c,CombatOp::Fire,"rifle",actor),blind).status==Status::MissingRequirement && Bytes(c)==ready,"Companion does not fire automatically without perceived danger or an intended distraction");
      const auto fire=ApplyW(c,W(c,CombatOp::Fire,"rifle",actor),ctx);
      Check(fire.sound && fire.shot && fire.shot->actor==actor && c.HunterRuntime().actors.empty(),"Shot makes real noise without revealing an unobserved identity");
      auto wrong=W(c,CombatOp::ResolveShot,"hunter","joe",fire.shot->serial); const auto shotBytes=Bytes(c);
      Check(c.ApplyCombat(wrong,ctx).status==Status::MissingRequirement && Bytes(c)==shotBytes,"Shot serial cannot be claimed by a different shooter");
      const auto hit=ApplyW(c,W(c,CombatOp::ResolveShot,"hunter",actor,fire.shot->serial),ctx);
      Check(hit.hunterInterrupted && c.CombatRuntime().companions.at(actor).loadedRounds==29,"Verified NPC hit interrupts with shared Hunter immunity and no HP"); Resume(c);
    }
  CampaignCore hurt(rules::Difficulty::Normal,202); Share(hurt,"harley","rifle"); Share(hurt,"harley","magazine");
  ApplyW(hurt,W(hurt,CombatOp::BeginReload,"rifle","harley"),CombatCtx());
  CompanionCommand injury{Id(),CompanionOp::Injury,"harley","harley","F4_START",hurt.Now()}; injury.argument=1; injury.physicallyVerified=true;
  Check(hurt.ApplyCompanion(injury,Party()).status==Status::Applied && !hurt.CombatRuntime().companions.at("harley").task,"Light injury cancels NPC preparation without consuming reserved ammo"); Resume(hurt);
}
std::uint64_t Smoke(CampaignCore& c) {
  const auto p=CombatProfile(c.Narrative().difficulty);
  const auto begin=ApplyW(c,W(c,CombatOp::BeginThrow,"smoke"),CombatCtx()); Tick(c,p.throwSeconds);
  ApplyW(c,W(c,CombatOp::ReleaseThrow,"smoke","joe",begin.task->serial),CombatCtx()); Tick(c,p.smokeFuse);
  ApplyW(c,W(c,CombatOp::Detonate,"smoke","joe",begin.task->serial),CombatCtx()); return begin.task->serial;
}
void SmokeLearning() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    CampaignCore c(mode,203); auto ctx=Ctx(); ctx.observedSmokeEdges["F4_START"].insert("F4_HALL_W");
    Check(c.HunterPlanOffers(ctx).empty(),"Hunter has no smoke pattern before observation");
    if(mode==rules::Difficulty::Punitive) for(const auto& pickup:data::Hospital().pickups) if(pickup.item=="smoke") {
      Check(c.Apply({Id(),ActionKind::Collect,pickup.id,pickup.node,c.Now(),0,true}).status==Status::Applied,"Second smoke has an actual source"); break;
    }
    for(int i=0;i<2;++i) {
      const auto serial=Smoke(c); auto notice=Q(c,StrategyOp::NoticeSmoke,"F4_START","F4_HALL_E",serial);
      auto unseen=ctx; unseen.lineOfSightVerified=false; RejectQ(c,notice,unseen); ApplyQ(c,notice,ctx);
      notice.eventId=Id(); RejectQ(c,notice,ctx); Resume(c);
      Check(c.StrategyRuntime().smokePatterns.at("F4_START").uses==static_cast<unsigned>(i+1),"Only distinct observed clouds train the pattern");
    }
    const auto offers=c.HunterPlanOffers(ctx); Check(!offers.empty() && c.HunterRuntime().actors.empty(),"Learned smoke offers a fallible edge watch without seeing through smoke");
    if(mode==rules::Difficulty::Normal) {
      auto bytes=Bytes(c); const auto first=bytes.find("RECEIPTS "), end=bytes.find('\n',first);
      const auto count=c.AppliedActionCount(); std::ostringstream padding; padding<<"RECEIPTS 9997\n";
      const auto circuit=*data::Hospital().circuitNodes.begin();
      for(std::size_t i=count;i<9997;++i) { Action a{Id(),ActionKind::Circuit,circuit.first,circuit.second,0,0,true}; padding<<"RECEIPT "<<std::quoted(a.eventId)<<' '<<std::quoted(detail::Signature(a))<<'\n'; }
      bytes.replace(first,end-first+1,padding.str()); bytes.resize(bytes.rfind("CHECKSUM ")); bytes+="CHECKSUM "+detail::Hash(bytes)+'\n';
      std::istringstream nearFull(bytes); Check(c.Load(nearFull)==save::Result::Ok,"Near-full receipt fixture keeps sources and state coherent");
    }
    auto begin=Q(c,StrategyOp::BeginSmokeWatch,"F4_START"); begin.destination="F4_HALL_W"; ApplyQ(c,begin,ctx); Resume(c);
    if(mode==rules::Difficulty::Normal) {
      const auto circuit=*data::Hospital().circuitNodes.begin(); const auto before=Bytes(c);
      Check(c.Apply({Id(),ActionKind::Circuit,circuit.first,circuit.second,c.Now(),0,true}).status==Status::LogFull && Bytes(c)==before,"Other families cannot spend the plan's final receipt budget");
    }
    const auto plan=c.StrategyRuntime().hunterPlan->serial; auto arrive=Q(c,StrategyOp::ArrivePlan,"F4_START","F4_HALL_W",plan);
    ctx.actualHunterNode=arrive.node; ApplyQ(c,arrive,ctx); Resume(c);
    auto finish=Q(c,StrategyOp::CompletePlanStep,"F4_START",arrive.node,plan); RejectQ(c,finish,ctx);
    Tick(c,HunterProfile(mode).watchSeconds); finish.atSeconds=c.Now(); ApplyQ(c,finish,ctx);
    Check(!c.StrategyRuntime().hunterPlan && c.HunterPlanOffers(ctx).empty() && c.Ending()==rules::Ending::None,"Watch ends with cooldown, no automatic hit or capture"); Resume(c);
  }
}
void Pressure(CampaignCore& c,HunterPressure type,const std::string& node) {
  HunterCommand a; a.eventId=Id(); a.kind=HunterOp::Pressure; a.node=node; a.atSeconds=c.Now(); a.argument=static_cast<int>(type);
  HunterContext ctx; ctx.actualHunterNode=node; ctx.physicallyVerified=ctx.lineOfSightVerified=true;
  Check(c.ApplyHunter(a,ctx).status==Status::Applied,"Perceived aggression source is real"); Bytes(c);
}
void TrapChain() {
  CampaignCore c(rules::Difficulty::Hard,204); const auto world=data::Hospital();
  const auto door=world.doors.front(); const auto circuit=*world.circuitNodes.begin();
  Check(c.Apply({Id(),ActionKind::OpenDoor,door.id,door.fromNode,0,0,true}).status==Status::Applied,"Observed passage is physically open");
  HunterContext hunter; hunter.actualHunterNode=door.fromNode; hunter.physicallyVerified=hunter.lineOfSightVerified=true;
  for(int i=0;i<2;++i) {
    HunterCommand see; see.kind=HunterOp::SeeActor; see.actor="joe"; see.node=door.fromNode; see.atSeconds=c.Now(); Check(c.SenseHunter(see,hunter),"Joe actually seen using the passage");
    HunterCommand pass; pass.eventId=Id(); pass.kind=HunterOp::NoticePassage; pass.actor="joe"; pass.target=door.id; pass.node=door.fromNode; pass.atSeconds=c.Now();
    Check(c.ApplyHunter(pass,hunter).status==Status::Applied,"Observed passage traversal is recorded"); if(!i) Tick(c,15);
  }
  Check(c.InspectEvidence(door.id,"open",{},true) && c.InspectEvidence(circuit.first,"on",{},true),"Trap uses only inspected device states");
  Pressure(c,HunterPressure::Explosion,door.fromNode); Tick(c,3); Pressure(c,HunterPressure::Explosion,door.fromNode);
  auto ctx=Ctx(door.fromNode); ctx.safeToAlter={door.id,circuit.first};
  auto start=Q(c,StrategyOp::BeginTrap,door.id,door.fromNode); start.reference=circuit.first; start.destination=door.fromNode;
  auto unsafe=ctx; unsafe.alternativePathVerified=false; RejectQ(c,start,unsafe); ApplyQ(c,start,ctx); Resume(c);
  const auto serial=c.StrategyRuntime().hunterPlan->serial;
  for(int step=0;step<3;++step) {
    const auto target=step==0 ? circuit.first : door.id; const auto node=step==0 ? circuit.second : door.fromNode; ctx.actualHunterNode=node;
    auto done=Q(c,StrategyOp::CompletePlanStep,target,node,serial,step); RejectQ(c,done,ctx);
    auto arrive=Q(c,StrategyOp::ArrivePlan,target,node,serial,step); auto quiet=ctx; quiet.reactionCueVerified=false; RejectQ(c,arrive,quiet); ApplyQ(c,arrive,ctx);
    Tick(c,step==2 ? HunterProfile(rules::Difficulty::Hard).watchSeconds : HunterProfile(rules::Difficulty::Hard).reactionSeconds); done.atSeconds=c.Now();
    if(step<2) { unsafe=ctx; unsafe.alternativePathVerified=false; RejectQ(c,done,unsafe); }
    ApplyQ(c,done,ctx); Resume(c);
  }
  Check(!c.StrategyRuntime().hunterPlan && c.Ending()==rules::Ending::None,"Three physical steps never auto-capture Joe");
}
void RetreatAndInterruption() {
  CampaignCore c(rules::Difficulty::Normal,205); Pressure(c,HunterPressure::Gunshot,"F4_HALL_E");
  auto ctx=Ctx(); ctx.observedCoverNodes.insert("F4_STORAGE"); auto begin=Q(c,StrategyOp::BeginRetreat,"F4_STORAGE"); begin.destination=begin.target;
  ApplyQ(c,begin,ctx); const auto plan=c.StrategyRuntime().hunterPlan->serial; Resume(c);
  HunterCommand see; see.kind=HunterOp::SeeActor; see.actor="joe"; see.node="F4_START"; see.atSeconds=c.Now();
  HunterContext h; h.actualHunterNode=ctx.actualHunterNode; h.physicallyVerified=h.lineOfSightVerified=true;
  Check(c.SenseHunter(see,h) && c.StrategyRuntime().hunterPlan->interrupted,"New actual contact interrupts the old tactical plan"); Resume(c);
  auto arrive=Q(c,StrategyOp::ArrivePlan,"F4_STORAGE","F4_STORAGE",plan); ctx.actualHunterNode=arrive.node; RejectQ(c,arrive,ctx);
  auto cancel=Q(c,StrategyOp::CancelPlan,"F4_STORAGE",ctx.actualHunterNode,plan); ApplyQ(c,cancel,ctx); Resume(c);
}
}
int main() { try { ActorWeapons(); SmokeLearning(); TrapChain(); RetreatAndInterruption(); std::cout<<"CampaignExtensions: "<<checks<<" checks passed\n"; } catch(const std::exception& e) { std::cerr<<"Extension failure: "<<e.what()<<'\n'; return 1; } }
