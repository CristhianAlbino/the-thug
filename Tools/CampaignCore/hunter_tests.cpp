#include "CampaignCore.h"
#include "CampaignIO.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace thug;
using namespace thug::campaign;
namespace {
int checks=0;
void Check(bool value,const char* reason){
  ++checks;
  if(!value)throw std::runtime_error(reason);
}
WorldDefinition Fixture(){
  WorldDefinition d;
  d.levelId="hunter_fixture";
  d.nodes={"F4_START","A","B","C"};
  d.objects={{"D","door",{"open",{10,0,0},{} ,true} }, {"DP","door",{"closed",{20,0,0},{} ,true} }, {"CIR","circuit",{"on",{20,0,0},{} ,true} }, {"CAM","camera",{"online",{30,0,0},{} ,true} }, {"HIDE","hide",{"empty",{10,0,0},{} ,true} } };
  d.doors={{"D","A","B","",world::DoorPower::Manual},{"DP","B","C","CIR",world::DoorPower::RequiresPower} };
  d.circuitNodes={{"CIR","B"} };
  d.cameras={{"CAM","C","A","CIR"} };
  d.hides={{"HIDE","A"} };
  return d;
}
HunterContext Context(std::string node="A"){
  HunterContext c;
  c.actualHunterNode=std::move(node);
  c.reachableNodes={"A","B","C"};
  c.physicallyVerified=true;
  c.lineOfSightVerified=true;
  c.alternativePathVerified=true;
  c.reactionCueVerified=true;
  c.safeToAlter={"D","CIR"};
  return c;
}
HunterCommand Command(const CampaignCore& c,std::string id,HunterOp op,std::string node="A"){
  HunterCommand x;
  x.eventId=std::move(id);
  x.kind=op;
  x.node=std::move(node);
  x.atSeconds=c.Now();
  return x;
}
std::string Bytes(const CampaignCore& c){
  std::ostringstream out;
  Check(c.Save(out)==save::Result::Ok,"Hunter runtime checkpoint coherent");
  return out.str();
}
void Load(CampaignCore& c,const std::string& bytes){
  std::istringstream in(bytes);
  Check(c.Load(in)==save::Result::Ok,"Hunter checkpoint resumes");
}
void Rejected(CampaignCore& c,const HunterCommand& x,const HunterContext& ctx,Status expected){
  const auto before=Bytes(c);
  Check(c.ApplyHunter(x,ctx).status==expected,"Hunter command rejected with expected status");
  Check(Bytes(c)==before,"Rejected Hunter command preserves all state and RNG");
}
void See(CampaignCore& c,const std::string& actor,const std::string& node,bool threat=false){
  auto x=Command(c,"view-"+actor,HunterOp::SeeActor,node);
  x.actor=actor;
  x.argument=threat;
  x.observedPosition={10,0,0};
  Check(c.SenseHunter(x,Context()),"Verified identity sighting accepted");
}
HunterTask Choose(CampaignCore& c,const HunterContext& ctx,const std::string& id){
  auto x=Command(c,id,HunterOp::Choose,ctx.actualHunterNode);
  auto result=c.ApplyHunter(x,ctx);
  Check(result.status==Status::Applied&&result.task.has_value(),"Hunter chooses reachable task");
  return *result.task;
}
HunterCommand TaskCommand(const CampaignCore& c,const HunterTask& t,HunterOp op,std::string id){
  auto x=Command(c,std::move(id),op,t.destination);
  x.target=t.target;
  x.taskSerial=t.serial;
  return x;
}
void Advance(CampaignCore& c,double seconds){
  Check(c.Tick(seconds,survival::Activity::Rest).status==Status::Applied,"Campaign advances Hunter clock");
}
void Raise(CampaignCore& c){
  for(int i=0;i<4;++i){
    auto x=Command(c,"blast-"+std::to_string(i),HunterOp::Pressure);
    x.argument=static_cast<int>(HunterPressure::Explosion);
    Check(c.ApplyHunter(x,Context()).status==Status::Applied,"Witnessed destructive event raises pressure");
    if(i<3)Advance(c,3);
  }
}
bool Has(const std::vector<HunterOffer>& offers,HunterGoal goal){
  return std::any_of(offers.begin(),offers.end(),[&](const auto& o){
    return o.goal==goal;
  });
}
HunterTask Pick(CampaignCore& c,HunterGoal wanted,HunterContext ctx,const std::string& prefix){
  for(int i=0;i<300;++i){
    auto task=Choose(c,ctx,prefix+std::to_string(i));
    if(task.goal==wanted)return task;
    auto x=TaskCommand(c,task,HunterOp::Cancel,"cancel-"+prefix+std::to_string(i));
    x.node=ctx.actualHunterNode;
    Check(c.ApplyHunter(x,ctx).status==Status::Applied,"Unwanted legal candidate cancelled");
  }
  throw std::runtime_error("Weighted policy did not choose tested goal");
}
void PerceptionAndTargets(){
  for(const auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}){
    CampaignCore c(mode,Fixture(),17);
    auto ctx=Context();
    Check(!Has(c.HunterOffers(ctx),HunterGoal::Pursue),"No target before identity is perceived");
    auto x=Command(c,"bad-sight",HunterOp::SeeActor,"B");
    x.actor="joe";
    auto blind=ctx;
    blind.lineOfSightVerified=false;
    Rejected(c,x,blind,Status::MissingRequirement);
    See(c,"joe","B");
    See(c,"harley","C",true);
    auto offers=c.HunterOffers(ctx);
    Check(offers.size()==2&&offers[1].weight<offers[0].weight,"Visible threatening companion can outrank Joe");
    bool joe=false,harley=false;
    for(int i=0;i<30;++i){
      const auto t=Choose(c,ctx,"choose-"+std::to_string(i));
      joe|=t.target=="joe";
      harley|=t.target=="harley";
      auto cancel=TaskCommand(c,t,HunterOp::Cancel,"cancel-"+std::to_string(i));
      cancel.node="A";
      Check(c.ApplyHunter(cancel,ctx).status==Status::Applied,"Target selection does not imply capture");
    }
    Check(joe&&harley&&c.Ending()==rules::Ending::None,"Both perceived actors can be chosen without automatic defeat");
    auto lost=Command(c,"lost",HunterOp::LoseActor);
    lost.actor="joe";
    Check(c.ApplyHunter(lost,ctx).status==Status::Applied,"Explicit visual loss accepted");
    Check(c.HunterRuntime().actors.at("joe").node=="B","Visual loss preserves last known node");
    ctx.reachableNodes={"B"};
    Check(!Has(c.HunterOffers(ctx),HunterGoal::Pursue),"Unreachable visible companion cannot become a pursuit destination");
    Advance(c,100);
    Check(c.Hunter().state==hunter::State::Patrol,"Long delta consumes visual lease and entire search, not a fresh search at frame end");
    Check(!c.HunterRuntime().actors.at("harley").visible,"Stale visual contact expires");
    CampaignCore restored(mode,Fixture(),999);
    Load(restored,Bytes(c));
    Check(Bytes(restored)==Bytes(c),"Identity memory and RNG restored exactly");
  }
  CampaignCore c(rules::Difficulty::Hard,Fixture());
  See(c,"joe","B");
  const auto t=Choose(c,Context(),"chase");
  See(c,"joe","C");
  Check(!c.HunterRuntime().task&&c.HunterRuntime().actors.at("joe").node=="C","New sighting cancels stale destination");
  Rejected(c,TaskCommand(c,t,HunterOp::Arrive,"stale-arrival"),Context("B"),Status::Conflict);
  auto stable=Choose(c,Context(),"stable-chase");
  See(c,"joe","C");
  Check(c.HunterRuntime().task&&c.HunterRuntime().task->serial==stable.serial,"Continuous sensor refresh preserves same destination task");
  Check(c.ApplyHunter(TaskCommand(c,stable,HunterOp::Arrive,"stable-arrive"),Context("C")).status==Status::Applied,"Chase can progress while vision refreshes");
  Check(c.ApplyHunter(TaskCommand(c,stable,HunterOp::Complete,"stable-done"),Context("C")).status==Status::Applied&&c.Ending()==rules::Ending::None,"Reaching seen actor is not an automatic attack");
  const auto receipts=c.AppliedActionCount();
  for(int i=0;i<12000;++i)See(c,"joe",i%2?"B":"C");
  Check(c.AppliedActionCount()==receipts&&c.HunterRuntime().latestSightings.size()==1,"Frequent sensing uses bounded memory without exhausting command receipts");
}
void PressureAndSearch(){
  CampaignCore c(rules::Difficulty::Hard,Fixture(),41);
  auto x=Command(c,"shot",HunterOp::Pressure);
  x.argument=0;
  auto blind=Context();
  blind.lineOfSightVerified=false;
  Rejected(c,x,blind,Status::MissingRequirement);
  Check(c.ApplyHunter(x,Context()).status==Status::Applied&&c.HunterRuntime().aggression==12,"Only witnessed pressure changes aggression");
  const auto once=Bytes(c);
  Check(c.ApplyHunter(x,Context()).status==Status::AlreadyApplied&&Bytes(c)==once,"Same pressure receipt is idempotent");
  auto repeat=x;
  repeat.eventId="same-frame";
  Rejected(c,repeat,Context(),Status::MissingRequirement);
  auto collision=x;
  collision.argument=1;
  Rejected(c,collision,Context(),Status::Conflict);
  Advance(c,10);
  Check(c.HunterRuntime().aggression<12,"Aggression decays on simulation clock");
  auto escape=Command(c,"unknown-escape",HunterOp::Pressure);
  escape.argument=3;
  escape.actor="jack";
  Rejected(c,escape,Context(),Status::MissingRequirement);
  c.SetPaused(true);
  const auto paused=Bytes(c);
  Check(c.Tick(10,survival::Activity::Rest).status==Status::Paused&&Bytes(c)==paused,"Pause freezes policy and memory");
  c.SetPaused(false);
  const hunter::LocationEstimate area{{10,0,0} ,5,c.Now()};
  Check(c.Observe({hunter::StimulusKind::Sound,hunter::Subject::Unknown,c.Now(),area,1,"","footstep"}),"Anonymous sound starts investigation");
  auto ctx=Context();
  ctx.searchSectors={{"B",{11,0,0} },{"C",{1000,0,0} } };
  auto offers=c.HunterOffers(ctx);
  Check(Has(offers,HunterGoal::Investigate)&&!Has(offers,HunterGoal::Pursue),"Unknown sound creates uncertain sector search without identity");
  for(const auto& o:offers)if(o.goal==HunterGoal::Investigate)Check(o.destination=="B","Sector outside uncertainty area excluded");
  auto t=Pick(c,HunterGoal::Investigate,ctx,"search-");
  auto arrive=TaskCommand(c,t,HunterOp::Arrive,"search-arrive");
  Check(c.ApplyHunter(arrive,Context(t.destination)).status==Status::Applied,"Navigation arrival acknowledged");
  Check(c.ApplyHunter(TaskCommand(c,t,HunterOp::Complete,"search-done"),Context(t.destination)).status==Status::Applied,"Search completion marks visited sector");
  Check(!Has(c.HunterOffers(ctx),HunterGoal::Investigate),"Empty searched sector not retried immediately");
  CampaignCore restored(rules::Difficulty::Hard,Fixture());
  Load(restored,Bytes(c));
  Check(Bytes(c)==Bytes(restored),"Search visit and pressure survive save");
}
void EnvironmentAndResume(){
  for(const auto goal:{HunterGoal::CloseDoor,HunterGoal::OpenDoor,HunterGoal::PowerOff,HunterGoal::DisableCamera}){
    CampaignCore c(rules::Difficulty::Hard,Fixture(),static_cast<int>(goal)+101);
    Raise(c);
    auto ctx=Context();
    Check(!Has(c.HunterOffers(ctx),goal),"Unseen device cannot be selected through global state");
    std::string id=goal==HunterGoal::PowerOff?"CIR":goal==HunterGoal::DisableCamera?"CAM":"D";
    if(goal==HunterGoal::OpenDoor){
      Action a{"close-fixture",ActionKind::CloseDoor,"D","A",c.Now(),0,true};
      Check(c.Apply(a).status==Status::Applied,"Fixture door closed by real command");
    }
    const auto state=c.World().objects.at(id).stateId;
    Check(c.InspectEvidence(id,state,{10,0,0} ,true),"Hunter sees device baseline");
    auto unsafe=ctx;
    unsafe.safeToAlter.clear();
    if(goal==HunterGoal::CloseDoor||goal==HunterGoal::PowerOff)Check(!Has(c.HunterOffers(unsafe),goal),"Blocking actions require alternative route at planning");
    const auto t=Pick(c,goal,ctx,"device-");
    Rejected(c,TaskCommand(c,t,HunterOp::Complete,"too-early"),Context(t.destination),Status::MissingRequirement);
    CampaignCore resumed(rules::Difficulty::Hard,Fixture(),999);
    Load(resumed,Bytes(c));
    Check(Bytes(c)==Bytes(resumed),"Travelling tactic restores without movement");
    auto noPath=Context(t.destination);
    noPath.reachableNodes.clear();
    Rejected(c,TaskCommand(c,t,HunterOp::Arrive,"no-path"),noPath,Status::MissingRequirement);
    auto silent=Context(t.destination);
    silent.reactionCueVerified=false;
    Rejected(c,TaskCommand(c,t,HunterOp::Arrive,"silent-arrival"),silent,Status::MissingRequirement);
    const auto arrival=TaskCommand(c,t,HunterOp::Arrive,"arrived");
    Check(c.ApplyHunter(arrival,Context(t.destination)).status==Status::Applied,"Physical arrival enters reaction phase");
    Rejected(c,TaskCommand(c,t,HunterOp::Complete,"no-window"),Context(t.destination),Status::MissingRequirement);
    Load(resumed,Bytes(c));
    Advance(c,3);
    Advance(resumed,3);
    Check(Bytes(c)==Bytes(resumed),"Reaction window continues across restart");
    auto unsafeFinish=Context(t.destination);
    unsafeFinish.alternativePathVerified=false;
    if(goal==HunterGoal::CloseDoor||goal==HunterGoal::PowerOff)Rejected(c,TaskCommand(c,t,HunterOp::Complete,"route-gone"),unsafeFinish,Status::MissingRequirement);
    auto finish=TaskCommand(c,t,HunterOp::Complete,"finished");
    Check(c.ApplyHunter(finish,Context(t.destination)).status==Status::Applied,"Observed device changed after physical arrival and reaction time");
    Check(c.ApplyHunter(finish,Context(t.destination)).status==Status::AlreadyApplied,"Environmental completion cannot replay effects");
    const auto changed=c.World().objects.at(id).stateId;
    Check(changed==(goal==HunterGoal::CloseDoor?"closed":goal==HunterGoal::OpenDoor?"open":goal==HunterGoal::PowerOff?"off":"interference"),"Device has intended native state");
    Load(resumed,Bytes(c));
    Check(Bytes(c)==Bytes(resumed),"Environmental consequence checkpoint coherent");
  }
  CampaignCore c(rules::Difficulty::Hard,Fixture(),50);
  Raise(c);
  Check(c.InspectEvidence("D","open",{} ,true),"Seen open door");
  auto t=Pick(c,HunterGoal::CloseDoor,Context(),"race-");
  Check(c.ApplyHunter(TaskCommand(c,t,HunterOp::Arrive,"race-arrive"),Context(t.destination)).status==Status::Applied,"Arrived before door race");
  Advance(c,3);
  Check(c.Apply({"external-close",ActionKind::CloseDoor,"D","A",c.Now(),0,true}).status==Status::Applied,"Another actor changes door while Hunter waits");
  Rejected(c,TaskCommand(c,t,HunterOp::Complete,"race-complete"),Context(t.destination),Status::MissingRequirement);
}
void AmbushAndHide(){
  CampaignCore c(rules::Difficulty::Punitive,Fixture(),7);
  auto ctx=Context();
  Check(!Has(c.HunterOffers(ctx),HunterGoal::WatchPassage)&&!Has(c.HunterOffers(ctx),HunterGoal::InspectHide),"No learned passage or guessed hidden spot");
  See(c,"joe","B");
  auto passage=Command(c,"passage",HunterOp::NoticePassage);
  passage.actor="joe";
  passage.target="D";
  Check(c.ApplyHunter(passage,ctx).status==Status::Applied,"Actually seen passage recorded");
  auto loss=Command(c,"lost",HunterOp::LoseActor);
  loss.actor="joe";
  Check(c.ApplyHunter(loss,ctx).status==Status::Applied,"Joe leaves sight");
  auto t=Pick(c,HunterGoal::WatchPassage,ctx,"watch-");
  Check(c.ApplyHunter(TaskCommand(c,t,HunterOp::Arrive,"watch-arrive"),Context(t.destination)).status==Status::Applied,"Ambush position reached without teleport");
  Advance(c,2);
  Check(c.ApplyHunter(TaskCommand(c,t,HunterOp::Complete,"watch-done"),Context(t.destination)).status==Status::Applied,"Watch stance starts after warning time");
  Check(!c.HunterRuntime().watchingDoor.empty()&&!Has(c.HunterOffers(ctx),HunterGoal::WatchPassage)&&c.Ending()==rules::Ending::None,"Ambush cooldown active, no automatic interception/capture");
  CampaignCore resumed(rules::Difficulty::Punitive,Fixture(),8);
  Load(resumed,Bytes(c));
  Check(Bytes(c)==Bytes(resumed),"Ambush stance and cooldown restored");
  Advance(c,50);
  Check(c.HunterRuntime().watchingDoor.empty(),"Watch stance expires");
  Check(c.Observe({hunter::StimulusKind::ConfirmedHideSpot,hunter::Subject::Player,c.Now(),{} ,1,"HIDE"}),"Hide use only remembered when observed");
  auto hide=Pick(c,HunterGoal::InspectHide,ctx,"hide-");
  Check(c.ApplyHunter(TaskCommand(c,hide,HunterOp::Arrive,"hide-arrive"),Context(hide.destination)).status==Status::Applied,"Hunter reaches remembered hide");
  Advance(c,2);
  const auto result=c.ApplyHunter(TaskCommand(c,hide,HunterOp::Complete,"hide-done"),Context(hide.destination));
  Check(result.status==Status::Applied&&result.inspectedHideOccupied==false,"Hunter can search an empty remembered hide");
  Check(c.Ending()==rules::Ending::None,"Empty hide inspection grants no attack");
  const auto valid=Bytes(c);
  auto corrupt=valid;
  const auto begin=corrupt.find("HUNTER_RUNTIME ");
  const auto end=corrupt.find('\n',begin);
  corrupt.replace(begin,end-begin,"HUNTER_RUNTIME 54 99 -1 0 \"\"");
  const auto checksum=corrupt.find("CHECKSUM ");
  corrupt=corrupt.substr(0,checksum)+"CHECKSUM "+detail::Hash(corrupt.substr(0,checksum))+"\n";
  std::istringstream in(corrupt);
  Check(resumed.Load(in)!=save::Result::Ok,"Rechecksummed inconsistent policy rejected transactionally");
  Load(resumed,valid);
  const auto before=Bytes(resumed);
  std::istringstream again(corrupt);
  Check(resumed.Load(again)!=save::Result::Ok&&Bytes(resumed)==before,"Failed policy restoration preserves current session");
}
void Determinism(){
  CampaignCore a(rules::Difficulty::Hard,Fixture(),9);
  Raise(a);
  Check(a.InspectEvidence("CAM","online",{} ,true),"Known camera for deterministic continuation");
  CampaignCore b(rules::Difficulty::Hard,Fixture(),999);
  Load(b,Bytes(a));
  for(int i=0;i<15;++i){
    auto x=Choose(a,Context(),"det-"+std::to_string(i));
    auto y=Choose(b,Context(),"det-"+std::to_string(i));
    Check(x.goal==y.goal&&x.target==y.target&&x.destination==y.destination,"Loaded weighted decisions continue same random stream");
    for(auto* c:{&a,&b}){
      auto cancel=TaskCommand(*c,x,HunterOp::Cancel,"det-cancel-"+std::to_string(i));
      cancel.node="A";
      Check(c->ApplyHunter(cancel,Context()).status==Status::Applied,"Deterministic task cancelled");
    }
    Check(Bytes(a)==Bytes(b),"Full campaign remains identical after policy cycle");
  }
}
}
int main(){
  try{
    PerceptionAndTargets();
    PressureAndSearch();
    EnvironmentAndResume();
    AmbushAndHide();
    Determinism();
    std::cout<<"Campaign Hunter: "<<checks<<" checks passed\n";
    return 0;
  }catch(const std::exception& e){
    std::cerr<<"Campaign Hunter failed: "<<e.what()<<" (check "<<checks<<")\n";
    return 1;
  }
}
