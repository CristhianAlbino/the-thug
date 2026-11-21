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
std::string Id() { return "strategy-test-"+std::to_string(++sequence); }
std::string Bytes(const CampaignCore& c) { std::ostringstream out; const auto result=c.Save(out); if(result!=save::Result::Ok) std::cerr<<"Snapshot failure at strategy serial "<<c.StrategyRuntime().serial<<" receipts "<<c.AppliedActionCount()<<" time "<<c.Now()<<"\n"; Check(result==save::Result::Ok,"Strategy checkpoint consistent across all systems"); return out.str(); }
void Resume(CampaignCore& c) {
  const auto bytes=Bytes(c); CampaignCore restored(c.Narrative().difficulty,999);
  std::istringstream in(bytes); Check(restored.Load(in)==save::Result::Ok,"Strategy restores atomically");
  Check(Bytes(restored)==bytes,"Restore preserves stock, intentions, knowledge and progress"); c=std::move(restored);
}
StrategyContext Context(const std::string& node="F4_START",bool hunter=false) {
  StrategyContext c; c.actualHunterNode=hunter?node:"F4_HALL_E";
  c.actionCompleted=c.areaObserved=c.speechPerceived=c.contactVerified=c.continuityVerified=true;
  for(const auto& name:{"joe","harley","bartholomeu","jack"}) c.actors.emplace(name,director::Actor{node,true});
  return c;
}
StrategyCommand Cmd(const CampaignCore& c,StrategyOp kind,const std::string& actor="joe",const std::string& target="",const std::string& node="F4_START") {
  StrategyCommand a; a.eventId=Id(); a.kind=kind; a.actor=actor; a.target=target; a.node=node; a.atSeconds=c.Now(); a.physicallyVerified=true; return a;
}
void Apply(CampaignCore& c,const StrategyCommand& a,const StrategyContext& ctx,bool accepted=true) {
  const auto result=c.ApplyStrategy(a,ctx); if(result.status!=Status::Applied || result.accepted!=accepted) std::cerr<<"Command "<<static_cast<int>(a.kind)<<" returned "<<static_cast<int>(result.status)<<" accepted "<<result.accepted<<'\n'; Check(result.status==Status::Applied && result.accepted==accepted,"Strategy command commits intended response");
  const auto bytes=Bytes(c); Check(c.ApplyStrategy(a,ctx).status==Status::AlreadyApplied && Bytes(c)==bytes,"Retried event cannot duplicate costs, information or consequences");
}
void Reject(CampaignCore& c,const StrategyCommand& a,const StrategyContext& ctx,Status expected=Status::MissingRequirement) {
  const auto bytes=Bytes(c); Check(c.ApplyStrategy(a,ctx).status==expected,"Invalid strategy command rejected"); Check(Bytes(c)==bytes,"Rejected command has no partial state");
}
void Tick(CampaignCore& c,double dt) { Check(c.Tick(dt,survival::Activity::Rest).status==Status::Applied,"Shared clock advances"); }
CompanionContext Party(const std::string& node) {
  CompanionContext c; c.actionCompleted=c.movementCompleted=true;
  for(const auto& a:{"joe","harley","bartholomeu","jack"}) c.actors.emplace(a,director::Actor{node,true});
  c.reachableNodes.insert(node); return c;
}
void Collect(CampaignCore& c,const std::string& pickup,const std::string& node) {
  Check(c.Apply({Id(),ActionKind::Collect,pickup,node,c.Now(),0,true}).status==Status::Applied,"World resource collected"); Bytes(c);
}
void Share(CampaignCore& c,const std::string& actor,const std::string& item,const std::string& node) {
  CompanionCommand a{Id(),CompanionOp::Transfer,"joe",actor,node,c.Now()}; a.item=item; a.quantity=1; a.physicallyVerified=true;
  Check(c.ApplyCompanion(a,Party(node)).status==Status::Applied,"Owned resource moves between bags"); Bytes(c);
}
void Contain(CampaignCore& c,const std::string& target) {
  const std::string node="F4_START";
  const auto see=[&] { HunterCommand a; a.kind=HunterOp::SeeActor; a.actor=target; a.node=node; a.atSeconds=c.Now();
    HunterContext ctx; ctx.actualHunterNode=node; ctx.physicallyVerified=ctx.lineOfSightVerified=true; Check(c.SenseHunter(a,ctx),"Prisoner actually seen"); };
  const auto command=[&](RescueOp op,std::uint64_t serial=0) { RescueCommand a; a.eventId=Id(); a.kind=op; a.actor="hunter"; a.target=target; a.node=node; a.atSeconds=c.Now(); a.serial=serial; a.physicallyVerified=true; return a; };
  RescueContext ctx; ctx.actualHunterNode=node; ctx.contactVerified=ctx.continuousContactVerified=ctx.actionCompleted=true;
  ctx.actors.emplace(target,director::Actor{node,true}); see();
  Check(c.ApplyRescue(command(RescueOp::BeginContainment),ctx)==Status::Applied,"Containment begins"); Tick(c,2); see();
  Check(c.ApplyRescue(command(RescueOp::FinishContainment,c.RescueRuntime().captures.at(target).serial),ctx)==Status::Applied,"Temporary containment completes"); Bytes(c);
}
void FloorResources() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    CampaignCore c(mode,101); auto ctx=Context();
    auto a=Cmd(c,StrategyOp::DropItem); a.item="magazine"; a.argument=1; a.position={1,2,3};
    auto ui=a; ui.physicallyVerified=false; Reject(c,ui,ctx,Status::InvalidAction);
    auto fake=ctx; fake.actionCompleted=false; Reject(c,a,fake);
    Apply(c,a,ctx); Check(c.KnownDrops("harley").empty() && c.KnownDrops("joe").size()==1,"Private floor knowledge is not broadcast"); Resume(c);
    auto take=Cmd(c,StrategyOp::TakeDrop,"harley",a.eventId); take.argument=1; Reject(c,take,ctx);
    auto notice=Cmd(c,StrategyOp::NoticeDrop,"harley",a.eventId); Apply(c,notice,ctx);
    notice.eventId=Id(); Reject(c,notice,ctx);
    Apply(c,take,ctx); Check(c.CompanionRuntime(companion::Character::Harley).inventory.contents.at("magazine")==1,"NPC claims real floor stock");
    Check(c.KnownDrops("joe").at(a.eventId).quantity==1,"Offscreen claim does not secretly update Joe's last observation");
    Apply(c,Cmd(c,StrategyOp::NoticeMissingDrop,"joe",a.eventId),ctx);
    Check(c.KnownDrops("joe").empty(),"Joe updates missing stock only on another physical inspection");
    auto race=Cmd(c,StrategyOp::TakeDrop,"joe",a.eventId); race.argument=1; Reject(c,race,ctx); Resume(c);
    auto drop=Cmd(c,StrategyOp::DropItem,"harley"); drop.item="magazine"; drop.argument=1; Apply(c,drop,ctx);
    Apply(c,Cmd(c,StrategyOp::NoticeDrop,"joe",drop.eventId),ctx);
    take=Cmd(c,StrategyOp::TakeDrop,"joe",drop.eventId); take.argument=1; Apply(c,take,ctx);
    Check(c.Companion(companion::Character::Harley).traits.ammunition==0,"Picking/dropping keeps trait ammunition synchronized");
    drop=Cmd(c,StrategyOp::DropItem); drop.item="rifle"; drop.argument=1; Reject(c,drop,ctx);
  }
}
void Information() {
  CampaignCore c(rules::Difficulty::Normal,102); auto ctx=Context();
  auto report=Cmd(c,StrategyOp::ReportThreat,"harley","joe"); report.destination="F4_HALL_E"; report.reference="unseen";
  Reject(c,report,ctx,Status::Conflict);
  auto observe=Cmd(c,StrategyOp::ObserveArea,"harley"); observe.destination=report.destination; observe.argument=1;
  auto blind=ctx; blind.areaObserved=false; Reject(c,observe,blind); Apply(c,observe,ctx);
  report.reference=observe.eventId; auto unheard=ctx; unheard.speechPerceived=false; Reject(c,report,unheard); Apply(c,report,ctx);
  Check(c.KnownReports("joe").size()==1 && c.JoeKnownRoutes().empty() && c.HunterRuntime().actors.empty(),"Report grants no route, identity track or secret information");
  Tick(c,11); Check(c.KnownReports("joe")[0].stale,"Old sighting remains visibly dated"); Resume(c);
  auto lie=Cmd(c,StrategyOp::ReportThreat,"joe","bartholomeu"); lie.destination="F4_STORAGE"; lie.argument=1;
  auto knownArea=Cmd(c,StrategyOp::ObserveArea); knownArea.destination=lie.destination; Apply(c,knownArea,ctx);
  Apply(c,lie,ctx); const auto trust=c.Companion(companion::Character::Bartholomeu).traits.trust;
  auto check=Cmd(c,StrategyOp::CheckReport,"bartholomeu",lie.eventId); check.reference="unseen"; Reject(c,check,ctx);
  auto inspect=Cmd(c,StrategyOp::ObserveArea,"bartholomeu"); inspect.destination=lie.destination; Apply(c,inspect,ctx);
  check.reference=inspect.eventId; Apply(c,check,ctx);
  Check(c.Companion(companion::Character::Bartholomeu).traits.trust==trust-15 && c.KnownReports("bartholomeu")[0].contradicted,"Actually contradicted manipulation has a one-time relationship cost");
  check.eventId=Id(); Reject(c,check,ctx); Resume(c);
  CampaignCore liar(rules::Difficulty::Hard,108);
  CompanionCommand abandon{Id(),CompanionOp::Abandon,"joe","jack","F4_START",0}; abandon.physicallyVerified=true;
  Check(liar.ApplyCompanion(abandon,Party("F4_START")).status==Status::Applied,"Observed abandonment creates a motive for self-preserving deception");
  auto area=Cmd(liar,StrategyOp::ObserveArea,"jack"); area.destination="F4_STORAGE"; Apply(liar,area,ctx);
  auto statement=Cmd(liar,StrategyOp::ReportThreat,"jack","joe"); statement.destination=area.destination; statement.argument=1; Apply(liar,statement,ctx);
  const auto loyalty=liar.Companion(companion::Character::Jack).traits.trust;
  auto verify=Cmd(liar,StrategyOp::ObserveArea); verify.destination=area.destination; Apply(liar,verify,ctx);
  auto expose=Cmd(liar,StrategyOp::CheckReport,"joe",statement.eventId); expose.reference=verify.eventId; Apply(liar,expose,ctx);
  Check(liar.KnownReports("joe")[0].contradicted && liar.Companion(companion::Character::Jack).traits.trust==loyalty,"Joe can expose an NPC's lie without magically changing the liar's trust in Joe"); Resume(liar);
  CampaignCore easy(rules::Difficulty::Easy); lie=Cmd(easy,StrategyOp::ReportThreat,"jack","joe"); lie.destination="F4_STORAGE"; lie.argument=1; Reject(easy,lie,ctx);
}
void Questions() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    CampaignCore c(mode,103); auto ctx=Context("F4_START",true);
    auto begin=Cmd(c,StrategyOp::BeginQuestion,"hunter","bartholomeu"); begin.serial=1; Reject(c,begin,ctx);
    Contain(c,"bartholomeu"); begin.atSeconds=c.Now(); begin.serial=c.RescueRuntime().serial; Apply(c,begin,ctx); Resume(c);
    auto answer=Cmd(c,StrategyOp::AnswerQuestion,"bartholomeu","hunter"); answer.serial=c.StrategyRuntime().question->serial;
    Reject(c,answer,ctx); Tick(c,4); answer.atSeconds=c.Now();
    auto noContact=ctx; noContact.continuityVerified=false; Reject(c,answer,noContact);
    Apply(c,answer,ctx); Check(c.KnownReports("hunter").empty() && !c.Narrative().outcomes.at("bartholomeu").captured,"Silence conveys no invented route and never punishes with terminal capture/death"); Resume(c);
  }
  CampaignCore c(rules::Difficulty::Punitive,104);
  CompanionCommand route{Id(),CompanionOp::ObserveRoute,"bartholomeu","tunnel","B1_TUNNEL",0}; route.physicallyVerified=true;
  Check(c.ApplyCompanion(route,Party(route.node)).status==Status::Applied,"Prisoner has own witnessed route knowledge");
  Contain(c,"bartholomeu"); auto begin=Cmd(c,StrategyOp::BeginQuestion,"hunter","bartholomeu"); begin.serial=c.RescueRuntime().serial; Apply(c,begin,Context("F4_START",true));
  Tick(c,4); auto answer=Cmd(c,StrategyOp::AnswerQuestion,"bartholomeu","hunter"); answer.serial=c.StrategyRuntime().question->serial; answer.argument=1; answer.item="tunnel";
  Apply(c,answer,Context("F4_START",true)); Check(c.KnownReports("hunter").size()==1 && c.KnownReports("joe").empty() && c.HunterRuntime().actors.count("joe")==0,"Interrogation gives a fallible route report solely to the listener"); Resume(c);
  begin=Cmd(c,StrategyOp::BeginQuestion,"hunter","bartholomeu"); begin.serial=c.RescueRuntime().serial; Apply(c,begin,Context("F4_START",true));
  CompanionCommand harm{Id(),CompanionOp::Injury,"bartholomeu","bartholomeu","F4_START",c.Now()}; harm.argument=1; harm.physicallyVerified=true;
  Check(c.ApplyCompanion(harm,Party("F4_START")).status==Status::Applied,"External injury interrupts speech");
  Check(c.StrategyRuntime().question->interrupted,"Interrupted speech never keeps old progress"); Resume(c);
  auto cancel=Cmd(c,StrategyOp::CancelQuestion,"hunter","bartholomeu"); cancel.serial=c.StrategyRuntime().question->serial; Apply(c,cancel,Context("F4_START",true)); Resume(c);
}
void Cooperation() {
  CampaignCore c(rules::Difficulty::Normal,105); const std::string node="F1_RADIO";
  Collect(c,"P_BATTERY","F2_SERVER"); Share(c,"harley","battery",node);
  Collect(c,"P_RADIO",node);
  auto begin=Cmd(c,StrategyOp::BeginJointUse,"joe","harley",node); begin.reference="install_battery";
  Apply(c,begin,Context(node)); Check(c.Inventory().contents.count("battery")==0 && c.CompanionRuntime(companion::Character::Harley).inventory.contents.at("battery")==1,"Joint action reserves its helper's resource"); Resume(c);
  auto done=Cmd(c,StrategyOp::CompleteJointUse,"joe","harley",node); done.serial=c.StrategyRuntime().joint->serial; Reject(c,done,Context(node));
  Check(c.Apply({Id(),ActionKind::OpenDoor,"D_F1_RADIO_E","F1_RADIO",c.Now(),0,true}).status==Status::MissingRequirement,"Busy Joe cannot operate a door through a second action family");
  Tick(c,3); done.atSeconds=c.Now(); auto lost=Context(node); lost.actors.erase("harley"); Reject(c,done,lost);
  Apply(c,done,Context(node)); Check(c.CompanionRuntime(companion::Character::Harley).inventory.contents.count("battery")==0,"Completion consumes exactly the helper's battery"); Resume(c);
  CampaignCore refuse(rules::Difficulty::Punitive,106); begin=Cmd(refuse,StrategyOp::BeginJointUse,"joe","jack","F2_CAMERAS"); begin.reference="clear_interference";
  Apply(refuse,begin,Context(begin.node),false); Check(!refuse.StrategyRuntime().joint,"Unwilling companion does not join a hidden mandatory plan"); Resume(refuse);
  CampaignCore interrupted(rules::Difficulty::Normal,109); begin=Cmd(interrupted,StrategyOp::BeginJointUse,"joe","harley","F2_CAMERAS"); begin.reference="clear_interference";
  Apply(interrupted,begin,Context(begin.node)); const auto serial=interrupted.StrategyRuntime().joint->serial;
  Check(interrupted.Apply({Id(),ActionKind::Injury,"joe",begin.node,interrupted.Now(),1,true}).status==Status::Applied,"Real wound interrupts joint work");
  Check(interrupted.StrategyRuntime().joint->interrupted,"Joint medical epoch is invalidated"); Resume(interrupted); Tick(interrupted,3);
  done=Cmd(interrupted,StrategyOp::CompleteJointUse,"joe","harley",begin.node); done.serial=serial; Reject(interrupted,done,Context(begin.node));
  done.kind=StrategyOp::CancelJointUse; done.eventId=Id(); Apply(interrupted,done,Context(begin.node)); Resume(interrupted);
}
void Corruption() {
  CampaignCore c(rules::Difficulty::Normal,107); auto a=Cmd(c,StrategyOp::DropItem); a.item="magazine"; a.argument=1; Apply(c,a,Context());
  const auto good=Bytes(c); auto bad=good; const auto at=bad.find("STRATEGY "); Check(at!=std::string::npos,"Save carries explicit strategy state fingerprint"); bad[bad.find(' ',at+9)+1]='f';
  const auto checksum=bad.rfind("CHECKSUM "); bad.resize(checksum); bad+="CHECKSUM "+detail::Hash(bad)+'\n';
  std::istringstream in(bad); Check(c.Load(in)==save::Result::IncompatibleRoster && Bytes(c)==good,"Rechecks derived state even after checksum recalculation");
}
}
int main() { try { FloorResources(); Information(); Questions(); Cooperation(); Corruption(); std::cout<<"CampaignStrategy: "<<checks<<" checks passed\n"; } catch(const std::exception& e) { std::cerr<<"Strategy failure: "<<e.what()<<'\n'; return 1; } }
