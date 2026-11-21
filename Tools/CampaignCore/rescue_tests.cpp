#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "RescueIO.h"
#include <iostream>
#include <stdexcept>
using namespace thug;
using namespace thug::campaign;
namespace {
int checks=0, sequence=0;
const std::string node="F4_START";
void Check(bool ok,const char* why) { ++checks; if(!ok) throw std::runtime_error(why); }
std::string Id() { return "rescue-test-"+std::to_string(++sequence); }
std::string Bytes(const CampaignCore& c) { std::ostringstream out; Check(c.Save(out)==save::Result::Ok,"Rescue checkpoint coherent"); return out.str(); }
void Resume(CampaignCore& c) {
  const auto bytes=Bytes(c); CampaignCore restored(c.Narrative().difficulty,999);
  std::istringstream in(bytes); Check(restored.Load(in)==save::Result::Ok,"Rescue restores atomically");
  Check(Bytes(restored)==bytes,"Load preserves stage, clock, serial, inventory and journal");
  c=std::move(restored);
}
void Tick(CampaignCore& c,double dt) { Check(c.Tick(dt,survival::Activity::Rest).status==Status::Applied,"Shared time advances"); }
RescueContext Context(bool hunterHere=false,bool witnessed=true) {
  RescueContext ctx; ctx.actualHunterNode=hunterHere?node:"F4_HALL_E";
  ctx.contactVerified=ctx.continuousContactVerified=ctx.openingVerified=ctx.actionCompleted=ctx.transferCompleted=true;
  ctx.joeWitnessedAction=witnessed;
  for(const auto& name:{"joe","harley","bartholomeu","jack"}) ctx.actors.emplace(name,director::Actor{node,true});
  return ctx;
}
RescueCommand Cmd(const CampaignCore& c,RescueOp kind,const std::string& actor="joe",const std::string& target="bartholomeu",std::uint64_t serial=0) {
  RescueCommand a; a.eventId=Id(); a.actor=actor; a.target=target; a.node=node; a.kind=kind;
  a.atSeconds=c.Now(); a.serial=serial; a.physicallyVerified=true; return a;
}
void Apply(CampaignCore& c,const RescueCommand& a,const RescueContext& ctx) {
  const auto result=c.ApplyRescue(a,ctx); Check(result==Status::Applied,"Physical rescue command commits"); Bytes(c);
  const auto bytes=Bytes(c); Check(c.ApplyRescue(a,ctx)==Status::AlreadyApplied && Bytes(c)==bytes,"Retried command changes nothing");
}
void Reject(CampaignCore& c,const RescueCommand& a,const RescueContext& ctx,Status expected=Status::MissingRequirement) {
  const auto bytes=Bytes(c); Check(c.ApplyRescue(a,ctx)==expected,"Invalid rescue command rejected");
  Check(Bytes(c)==bytes,"Rejection consumes no item, receipt, clock or outcome");
}
void See(CampaignCore& c,const std::string& actor) {
  HunterCommand a; a.kind=HunterOp::SeeActor; a.actor=actor; a.node=node; a.atSeconds=c.Now();
  HunterContext ctx; ctx.actualHunterNode=node; ctx.physicallyVerified=ctx.lineOfSightVerified=true;
  ctx.reachableNodes.insert(node); Check(c.SenseHunter(a,ctx),"Hunter actually sees identified companion");
}
std::uint64_t Contain(CampaignCore& c,const std::string& actor="bartholomeu",bool witnessed=true) {
  See(c,actor); auto begin=Cmd(c,RescueOp::BeginContainment,"hunter",actor);
  Apply(c,begin,Context(true,witnessed)); const auto serial=c.RescueRuntime().captures.at(actor).serial;
  Check(!c.Narrative().outcomes.at(actor).captured,"Restraint intention is not confirmed capture");
  Resume(c); Tick(c,RestrainSeconds); See(c,actor);
  Apply(c,Cmd(c,RescueOp::FinishContainment,"hunter",actor,serial),Context(true,witnessed));
  Check(!c.Narrative().outcomes.at(actor).captured,"Temporary containment remains recoverable");
  return serial;
}
CompanionContext Party() {
  CompanionContext ctx; ctx.actionCompleted=ctx.movementCompleted=true; ctx.reachableNodes.insert(node);
  for(const auto& name:{"joe","harley","bartholomeu","jack"})ctx.actors.emplace(name,director::Actor{node,true});
  return ctx;
}
void Joe(CampaignCore& c,ActionKind kind,const std::string& target,const std::string& where,int arg=0) {
  Check(c.Apply({Id(),kind,target,where,c.Now(),arg,true}).status==Status::Applied,"Joe physical fixture commits"); Bytes(c);
}
void PhysicalGates() {
  CampaignCore c(rules::Difficulty::Normal);
  auto a=Cmd(c,RescueOp::BeginContainment,"hunter");
  Reject(c,a,Context(true)); See(c,a.target);
  auto noContact=Context(true); noContact.contactVerified=false; Reject(c,a,noContact);
  auto elsewhere=Context(); Reject(c,a,elsewhere);
  a.physicallyVerified=false; Reject(c,a,Context(true),Status::InvalidAction); a.physicallyVerified=true;
  Apply(c,a,Context(true)); const auto serial=c.RescueRuntime().captures.at(a.target).serial;
  HunterContext hunter; hunter.actualHunterNode=node; hunter.physicallyVerified=true; hunter.reachableNodes.insert(node);
  Check(c.HunterOffers(hunter).empty(),"Hunter cannot choose another trip while physically restraining");
  auto finish=Cmd(c,RescueOp::FinishContainment,"hunter",a.target,serial);
  Reject(c,finish,Context(true)); Tick(c,2); See(c,a.target); finish.atSeconds=c.Now();
  auto broken=Context(true); broken.continuousContactVerified=false; Reject(c,finish,broken);
  auto foreign=finish; ++foreign.serial; Reject(c,foreign,Context(true));
  Check(c.CompanionOffers(a.target,node,Party()).empty(),"Restrained companion has no movement/loot offers");
  Check(c.Tick(1,survival::Activity::Rest,{{a.target,survival::Activity::Sprint}}).status==Status::MissingRequirement,"Restrained body cannot sprint");
  auto cancel=Cmd(c,RescueOp::CancelContainment,"hunter",a.target,serial); Reject(c,cancel,Context(true));
  broken.contactLostVerified=true; Apply(c,cancel,broken);
  Check(c.RescueRuntime().captures.at(a.target).phase==CapturePhase::Released,"Lost contact releases incomplete restraint");
  Check(c.NightRecord().empty(),"Unfinished restraint never confirms a fate in Joe's record");
  Resume(c);
}
void ReleaseAcrossModes() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive})
    for(const auto& target:{"harley","bartholomeu","jack"}) {
      CampaignCore c(mode,13); Contain(c,target);
      auto a=Cmd(c,RescueOp::BeginRelease,"joe",target); Apply(c,a,Context());
      const auto serial=c.RescueRuntime().captures.at(target).release->serial;
      auto done=Cmd(c,RescueOp::CompleteRelease,"joe",target,serial); Reject(c,done,Context());
      Tick(c,5); const auto at=c.Now(); c.SetPaused(true);
      Check(c.Tick(100,survival::Activity::Rest).status==Status::Paused && c.Now()==at,"Pause consumes no release time");
      done.atSeconds=c.Now(); Reject(c,done,Context(),Status::Paused); c.SetPaused(false);
      Resume(c); Tick(c,5); done.atSeconds=c.Now();
      auto noContact=Context(); noContact.continuousContactVerified=false; Reject(c,done,noContact);
      auto absent=Context(); absent.actors.erase(target); Reject(c,done,absent);
      Reject(c,done,Context(true));
      Apply(c,done,Context());
      Check(c.RescueRuntime().captures.at(target).phase==CapturePhase::Released && !c.Narrative().outcomes.at(target).captured,"Manual release succeeds in all modes");
      Check(c.KnownCompanionFates().at(target)=="paradeiro desconhecido","Release does not guarantee later escape");
      Check(c.JoeKnownRoutes().empty(),"Rescue grants no unexplored exit");
      Check(!c.HunterRuntime().actors.count("joe"),"Helping never gives Hunter Joe's hidden location");
      Resume(c);
    }
}
void ToolAndCancellation() {
  CampaignCore c(rules::Difficulty::Normal); Joe(c,ActionKind::Collect,"P_WINDOW_TOOL","F2_SUPPLY"); Contain(c);
  auto a=Cmd(c,RescueOp::BeginRelease); a.item="window_tool"; Apply(c,a,Context());
  auto task=*c.RescueRuntime().captures.at(a.target).release;
  Check(c.Inventory().contents.at("window_tool")==1,"Tool remains reserved before completion");
  CompanionCommand give{Id(),CompanionOp::Transfer,"joe","jack",node,c.Now()}; give.item="window_tool"; give.quantity=1; give.physicallyVerified=true;
  const auto before=Bytes(c); Check(c.ApplyCompanion(give,Party()).status==Status::MissingRequirement && Bytes(c)==before,"Reserved tool cannot be transferred");
  Check(c.Tick(1,survival::Activity::Walk).status==Status::MissingRequirement,"Joe must cancel before walking away");
  Apply(c,Cmd(c,RescueOp::CancelRelease,"joe",a.target,task.serial),Context());
  Check(c.Inventory().contents.at("window_tool")==1,"Cancellation returns reservation without consumption");
  a.eventId=Id(); Apply(c,a,Context()); task=*c.RescueRuntime().captures.at(a.target).release;
  Tick(c,2); Resume(c); Tick(c,2);
  Apply(c,Cmd(c,RescueOp::CompleteRelease,"joe",a.target,task.serial),Context());
  Check(!c.Inventory().contents.count("window_tool"),"Completed tool attempt consumes exactly one real tool");
  Resume(c);
  CampaignCore missing(rules::Difficulty::Hard); Contain(missing);
  a=Cmd(missing,RescueOp::BeginRelease); a.item="window_tool"; Reject(missing,a,Context());
  Check(missing.RescueOffers("joe",Context()).size()==1,"Bare-handed attempt remains available without resources");
  CampaignCore npc(rules::Difficulty::Hard); Joe(npc,ActionKind::Collect,"P_WINDOW_TOOL","F2_SUPPLY");
  give={Id(),CompanionOp::Transfer,"joe","harley",node,npc.Now()}; give.item="window_tool"; give.quantity=1; give.physicallyVerified=true;
  Check(npc.ApplyCompanion(give,Party()).status==Status::Applied,"Harley receives her own real tool");
  Contain(npc); a=Cmd(npc,RescueOp::BeginRelease,"harley"); a.item="window_tool"; Apply(npc,a,Context());
  task=*npc.RescueRuntime().captures.at(a.target).release; Tick(npc,2); Resume(npc); Tick(npc,2);
  Apply(npc,Cmd(npc,RescueOp::CompleteRelease,"harley",a.target,task.serial),Context());
  Check(!npc.CompanionRuntime(companion::Character::Harley).inventory.contents.count("window_tool"),"Harley's release spends her own tool, never Joe's bag"); Resume(npc);
}
void AgencyAndUnknown() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    CampaignCore c(mode); Contain(c,"bartholomeu",false);
    Check(c.NightRecord().empty(),"Unseen containment is private world state");
    const auto offers=c.RescueOffers("bartholomeu",Context(false,false));
    Check(offers.size()==1 && offers.front().kind==RescueOp::BeginSelfRelease,"Fearful adult can choose an executable self release");
    auto a=Cmd(c,RescueOp::BeginSelfRelease,"bartholomeu"); Apply(c,a,Context(false,false));
    const auto serial=c.RescueRuntime().captures.at(a.target).release->serial;
    auto concurrent=Cmd(c,RescueOp::BeginRelease); Reject(c,concurrent,Context());
    Tick(c,SelfReleaseSeconds); Resume(c);
    Apply(c,Cmd(c,RescueOp::CompleteRelease,a.actor,a.target,serial),Context(false,false));
    Check(c.NightRecord().empty() && c.Companion(companion::Character::Bartholomeu).knownRoutes.empty(),"Unseen own release neither informs Joe nor invents route memory");
    CompanionCommand observe{Id(),CompanionOp::ObserveHide,a.target,"H_F4_SHEETS","F4_STORAGE",c.Now()};
    observe.physicallyVerified=true; auto free=Party(); free.actors.at(a.target).node=observe.node; free.reachableNodes.insert(observe.node);
    Check(c.ApplyCompanion(observe,free).status==Status::Applied,"Released companion regains ordinary perception and movement");
    Resume(c);
  }
  CampaignCore help(rules::Difficulty::Punitive); Contain(help);
  Check(help.RescueOffers("jack",Context()).empty(),"Jack's low loyalty can refuse helping another person");
  Check(!help.RescueOffers("harley",Context()).empty(),"Harley can choose to help Barth when physically present");
  auto a=Cmd(help,RescueOp::BeginRelease,"harley"); Apply(help,a,Context());
  Tick(help,ManualReleaseSeconds); const auto serial=help.RescueRuntime().captures.at(a.target).release->serial;
  Apply(help,Cmd(help,RescueOp::CompleteRelease,"harley",a.target,serial),Context());
  Resume(help);
}
void InjuryAndFinalCapture() {
  CampaignCore c(rules::Difficulty::Normal); Joe(c,ActionKind::Collect,"P_WINDOW_TOOL","F2_SUPPLY"); Contain(c);
  auto a=Cmd(c,RescueOp::BeginRelease); a.item="window_tool"; Apply(c,a,Context());
  const auto serial=c.RescueRuntime().captures.at(a.target).release->serial;
  Joe(c,ActionKind::Injury,"joe",node,1);
  Check(c.RescueRuntime().captures.at(a.target).release->interrupted,"Injury interrupts committed release even at the same timestamp");
  Tick(c,4); Resume(c); Reject(c,Cmd(c,RescueOp::CompleteRelease,"joe",a.target,serial),Context());
  Apply(c,Cmd(c,RescueOp::CancelRelease,"joe",a.target,serial),Context());
  Check(c.Inventory().contents.at("window_tool")==1,"Interrupted attempt wastes no reserved tool");
  // Treatment cannot revive progress that was invalidated by an injury.
  CampaignCore heal(rules::Difficulty::Easy); Contain(heal); Apply(heal,Cmd(heal,RescueOp::BeginRelease),Context());
  const auto old=heal.RescueRuntime().captures.at("bartholomeu").release->serial;
  Joe(heal,ActionKind::Injury,"joe",node,1); Joe(heal,ActionKind::Treat,"treatment",node);
  Check(heal.RescueRuntime().captures.at("bartholomeu").release->interrupted,"Healing does not resume interrupted work");
  Resume(heal); Tick(heal,10); Reject(heal,Cmd(heal,RescueOp::CompleteRelease,"joe","bartholomeu",old),Context());
  Apply(heal,Cmd(heal,RescueOp::CancelRelease,"joe","bartholomeu",old),Context());
  auto restart=Cmd(heal,RescueOp::BeginRelease); Apply(heal,restart,Context());
  const auto fresh=heal.RescueRuntime().captures.at(restart.target).release->serial;
  Reject(heal,Cmd(heal,RescueOp::CompleteRelease,"joe",restart.target,old),Context());
  Reject(heal,Cmd(heal,RescueOp::CompleteRelease,"joe",restart.target,fresh),Context());
  Tick(heal,10); Apply(heal,Cmd(heal,RescueOp::CompleteRelease,"joe",restart.target,fresh),Context()); Resume(heal);
  CampaignCore final(rules::Difficulty::Hard); const auto capture=Contain(final,"jack",false);
  Tick(final,100); Check(!final.Narrative().outcomes.at("jack").captured,"Elapsed time never finalizes capture");
  See(final,"jack"); auto command=Cmd(final,RescueOp::FinalizeCapture,"hunter","jack",capture);
  auto incomplete=Context(true,false); incomplete.transferCompleted=false; Reject(final,command,incomplete);
  Apply(final,command,Context(true,false));
  Check(final.Narrative().outcomes.at("jack").captured && !final.Narrative().outcomes.at("jack").dead,"Physical transfer confirms capture without death");
  Check(final.KnownCompanionFates().at("jack")=="paradeiro desconhecido" && final.NightRecord().empty(),"Offscreen final capture remains unknown to Joe");
  Reject(final,Cmd(final,RescueOp::BeginRelease,"joe","jack"),Context());
  Resume(final);
  CampaignCore known(rules::Difficulty::Normal); const auto t=Contain(known); Tick(known,8); See(known,"bartholomeu");
  Apply(known,Cmd(known,RescueOp::BeginRelease),Context());
  const auto pending=known.RescueRuntime().captures.at("bartholomeu").release->serial;
  Apply(known,Cmd(known,RescueOp::FinalizeCapture,"hunter","bartholomeu",t),Context(true));
  Check(known.KnownCompanionFates().at("bartholomeu")=="captura testemunhada","Joe records only witnessed final capture");
  Reject(known,Cmd(known,RescueOp::CompleteRelease,"joe","bartholomeu",pending),Context());
  Check(!known.RescueRuntime().captures.at("bartholomeu").release,"Final transfer cancels competing rescue once");
  Resume(known);
  CampaignCore ambush(rules::Difficulty::Normal); Contain(ambush);
  Apply(ambush,Cmd(ambush,RescueOp::BeginRelease,"harley"),Context());
  const auto helping=ambush.RescueRuntime().captures.at("bartholomeu").release->serial;
  See(ambush,"harley"); Apply(ambush,Cmd(ambush,RescueOp::BeginContainment,"hunter","harley"),Context(true));
  Check(!ambush.RescueRuntime().captures.at("bartholomeu").release,"Physical contact with helper interrupts her rescue");
  Reject(ambush,Cmd(ambush,RescueOp::CompleteRelease,"harley","bartholomeu",helping),Context());
  Resume(ambush);
  CampaignCore down(rules::Difficulty::Hard); const auto held=Contain(down);
  CompanionCommand hurt{Id(),CompanionOp::Injury,"bartholomeu","bartholomeu",node,down.Now()};
  hurt.argument=3; hurt.physicallyVerified=true;
  Check(down.ApplyCompanion(hurt,Party()).status==Status::Applied,"Real wound applied to restrained person");
  Tick(down,201); Check(down.CompanionRuntime(companion::Character::Bartholomeu).survival.collapsed,"Bleeding can cause real collapse without rescue punishment");
  See(down,"bartholomeu"); Apply(down,Cmd(down,RescueOp::FinalizeCapture,"hunter","bartholomeu",held),Context(true));
  Check(down.Narrative().outcomes.at("bartholomeu").captured && !down.Narrative().outcomes.at("bartholomeu").dead,"Incapacitated living person can still be physically captured"); Resume(down);
  CampaignCore joeCaught(rules::Difficulty::Normal); Contain(joeCaught);
  Apply(joeCaught,Cmd(joeCaught,RescueOp::BeginRelease),Context());
  const auto working=joeCaught.RescueRuntime().captures.at("bartholomeu").release->serial;
  Joe(joeCaught,ActionKind::Trigger,"captured",node);
  Check(joeCaught.Ending()==rules::Ending::Captured && joeCaught.RescueRuntime().captures.at("bartholomeu").release->interrupted,"Joe has no capture immunity while helping");
  Reject(joeCaught,Cmd(joeCaught,RescueOp::CompleteRelease,"joe","bartholomeu",working),Context(),Status::Finished);
  const auto terminalRecord=joeCaught.NightRecord().size();
  Apply(joeCaught,Cmd(joeCaught,RescueOp::CancelRelease,"joe","bartholomeu",working),Context());
  Check(joeCaught.NightRecord().size()==terminalRecord,"Technical cleanup after ending never narrates a new Joe action");
  Check(!joeCaught.Narrative().outcomes.at("bartholomeu").dead && !joeCaught.Narrative().outcomes.at("bartholomeu").captured,"Joe's defeat does not invent a companion's fate");
  Resume(joeCaught);
  CampaignCore interrupted(rules::Difficulty::Normal); Contain(interrupted);
  Apply(interrupted,Cmd(interrupted,RescueOp::BeginRelease),Context());
  const auto attempt=interrupted.RescueRuntime().captures.at("bartholomeu").release->serial;
  hurt={Id(),CompanionOp::Injury,"bartholomeu","bartholomeu",node,interrupted.Now()}; hurt.argument=1; hurt.physicallyVerified=true;
  Check(interrupted.ApplyCompanion(hurt,Party()).status==Status::Applied,"Victim's own wound interrupts joint action");
  Check(interrupted.RescueRuntime().captures.at("bartholomeu").release->interrupted,"Victim health change invalidates progress"); Resume(interrupted);
  Apply(interrupted,Cmd(interrupted,RescueOp::CancelRelease,"joe","bartholomeu",attempt),Context());
  CampaignCore deceased(rules::Difficulty::Normal); See(deceased,"bartholomeu");
  Apply(deceased,Cmd(deceased,RescueOp::BeginContainment,"hunter"),Context(true));
  const auto grapple=deceased.RescueRuntime().captures.at("bartholomeu").serial;
  hurt={Id(),CompanionOp::Injury,"bartholomeu","bartholomeu",node,deceased.Now()}; hurt.argument=3; hurt.physicallyVerified=true;
  Check(deceased.ApplyCompanion(hurt,Party()).status==Status::Applied,"Existing wound has a real cause");
  Tick(deceased,287); Check(deceased.Narrative().outcomes.at("bartholomeu").dead,"Actual fatal bleeding confirmed");
  auto lost=Context(true); lost.contactLostVerified=true;
  Apply(deceased,Cmd(deceased,RescueOp::CancelContainment,"hunter","bartholomeu",grapple),lost);
  Check(deceased.Narrative().outcomes.at("bartholomeu").dead && !deceased.Narrative().outcomes.at("bartholomeu").captured,"Cancelling contact does not resurrect or recapture the dead");
  Resume(deceased);
}
std::string Rehash(std::string s) { s.resize(s.rfind("CHECKSUM ")); return s+"CHECKSUM "+detail::Hash(s)+'\n'; }
void PersistenceAndBudget() {
  CampaignCore c(rules::Difficulty::Normal); Contain(c); Apply(c,Cmd(c,RescueOp::BeginRelease),Context());
  Tick(c,3); const auto good=Bytes(c);
  const auto reject=[&](std::string bad) {
    const auto before=Bytes(c); bad=Rehash(bad); std::istringstream in(bad);
    Check(c.Load(in)!=save::Result::Ok && Bytes(c)==before,"Forged rescue progress rejects entire checkpoint");
  };
  auto bad=good; const auto release=bad.find("RELEASE "); const auto end=bad.find('\n',release);
  auto line=bad.substr(release,end-release); const auto ready=line.rfind(" 2 12 0 0"); Check(ready!=std::string::npos,"Fixture preserves explicit release times");
  line.replace(ready,10," 2 3 0 0"); bad.replace(release,end-release,line); reject(bad);
  bad=good; const auto capture=bad.find("CAPTURE \"bartholomeu\" 1"); bad.replace(capture,std::string("CAPTURE \"bartholomeu\" 1").size(),"CAPTURE \"bartholomeu\" 3"); reject(bad);
  bad=good; bad.replace(0,std::string("THE_THUG_RUNTIME_CAMPAIGN 9").size(),"THE_THUG_RUNTIME_CAMPAIGN 7");
  std::istringstream legacy(bad); Check(c.Load(legacy)==save::Result::UnsupportedVersion,"Prior version is explicitly rejected");
  CampaignCore budget(rules::Difficulty::Normal); See(budget,"bartholomeu");
  auto bytes=Bytes(budget); const auto start=bytes.find("RECEIPTS "); const auto stop=bytes.find('\n',start);
  std::ostringstream padding; padding<<"RECEIPTS 9995\n";
  const auto circuit=*data::Hospital().circuitNodes.begin();
  for(int i=0;i<9995;++i) {
    Action action{Id(),ActionKind::Circuit,circuit.first,circuit.second,0,0,true};
    padding<<"RECEIPT "<<std::quoted(action.eventId)<<' '<<std::quoted(detail::Signature(action))<<'\n';
  }
  bytes.replace(start,stop-start+1,padding.str()); bytes=Rehash(bytes);
  std::istringstream in(bytes); Check(budget.Load(in)==save::Result::Ok,"Near-capacity receipt fixture restores");
  const auto t=Contain(budget);
  Apply(budget,Cmd(budget,RescueOp::BeginRelease),Context());
  auto extra=Cmd(budget,RescueOp::BeginContainment,"hunter","jack"); See(budget,"jack"); Reject(budget,extra,Context(true),Status::LogFull);
  Tick(budget,10); const auto releaseSerial=budget.RescueRuntime().captures.at("bartholomeu").release->serial;
  Apply(budget,Cmd(budget,RescueOp::CompleteRelease,"joe","bartholomeu",releaseSerial),Context());
  Check(budget.AppliedActionCount()==9999,"Reserved completion remains possible at receipt limit");
  Check(t>0,"Capture serial persists near capacity"); Resume(budget);
}
}
int main() {
  try { PhysicalGates(); ReleaseAcrossModes(); ToolAndCancellation(); AgencyAndUnknown(); InjuryAndFinalCapture(); PersistenceAndBudget();
    std::cout<<"CampaignRescue: "<<checks<<" checks passed\n"; return 0;
  } catch(const std::exception& e) { std::cerr<<"CampaignRescue: "<<e.what()<<" (check "<<checks<<")\n"; return 1; }
}
