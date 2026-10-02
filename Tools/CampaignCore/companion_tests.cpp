#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "CompanionData.generated.h"
#include "CompanionIO.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace thug;
using namespace thug::campaign;
namespace {
int checks=0,events=0;
void Check(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
std::string Id(){return "party_"+std::to_string(++events);}
std::string Bytes(const CampaignCore& c){std::ostringstream out;Check(c.Save(out)==save::Result::Ok,"Individual campaign checkpoint is coherent");return out.str();}
void Restore(CampaignCore& c,const std::string& bytes){std::istringstream in(bytes);Check(c.Load(in)==save::Result::Ok,"Individual state and tasks resume");Check(Bytes(c)==bytes,"Complete checkpoint roundtrips byte-exactly");}
CompanionCommand Cmd(const CampaignCore& c,CompanionOp kind,std::string actor,std::string target,std::string node,int argument=0){CompanionCommand a{Id(),kind,std::move(actor),std::move(target),std::move(node),c.Now()};a.argument=argument;a.physicallyVerified=true;return a;}
CompanionContext Context(const std::string& actor,const std::string& node){CompanionContext ctx;ctx.actors.emplace(actor,director::Actor{node});ctx.reachableNodes.insert(node);ctx.movementCompleted=ctx.actionCompleted=true;return ctx;}
void Together(CompanionContext& ctx,const std::string& node){for(const auto* name:{"joe","harley","bartholomeu","jack"})ctx.actors[name]={node};ctx.reachableNodes.insert(node);}
void Accepted(CampaignCore& c,const CompanionCommand& a,const CompanionContext& ctx){Check(c.ApplyCompanion(a,ctx).status==Status::Applied,"Companion transaction commits");Bytes(c);}
void Reject(CampaignCore& c,const CompanionCommand& a,const CompanionContext& ctx,Status status=Status::MissingRequirement){const auto before=Bytes(c);Check(c.ApplyCompanion(a,ctx).status==status,"Bad companion transaction is rejected");Check(Bytes(c)==before,"Failure preserves inventories, health, RNG, tasks and receipts");}
int Quantity(const survival::InventorySnapshot& bag,const std::string& item){const auto it=bag.contents.find(item);return it==bag.contents.end()?0:it->second;}
CompanionRuntimeSnapshot Body(const CampaignCore& c,const std::string& actor){return c.CompanionRuntime(*detail::Person(actor));}
void Tick(CampaignCore& c,double seconds){Check(c.Tick(seconds,survival::Activity::Rest).status==Status::Applied,"Shared simulation advances");}
void Joe(CampaignCore& c,ActionKind kind,const std::string& target,const std::string& node,int argument=0){Check(c.Apply({Id(),kind,target,node,c.Now(),argument,true}).status==Status::Applied,"Joe physical action commits");}
void Begin(CampaignCore& c,const std::string& actor,companion::Goal goal,const std::string& target,const std::string& node,CompanionContext ctx){Accepted(c,Cmd(c,CompanionOp::BeginTask,actor,target,node,static_cast<int>(goal)),ctx);Check(Body(c,actor).task->phase==TaskPhase::Travelling,"Selection does not finish travel");}
void Arrive(CampaignCore& c,const std::string& actor,CompanionContext ctx){const auto task=*Body(c,actor).task;ctx.actors[actor]={task.destination};ctx.reachableNodes.insert(task.destination);auto a=Cmd(c,CompanionOp::Arrive,actor,task.target,task.destination);a.taskSerial=task.serial;Accepted(c,a,ctx);}
CompanionCommand Completion(const CampaignCore& c,const std::string& actor){const auto task=*Body(c,actor).task;auto a=Cmd(c,CompanionOp::Complete,actor,task.target,task.destination,static_cast<int>(task.goal));a.taskSerial=task.serial;return a;}
void Finish(CampaignCore& c,const std::string& actor,CompanionContext ctx){const auto task=*Body(c,actor).task;ctx.actors[actor]={task.destination};ctx.reachableNodes.insert(task.destination);Arrive(c,actor,ctx);Tick(c,partydata::InteractionSeconds[static_cast<int>(task.goal)]);Accepted(c,Completion(c,actor),ctx);Check(!Body(c,actor).task,"Completion releases reservation");}
void Collect(CampaignCore& c,const std::string& actor,const std::string& pickup,const std::string& node){auto ctx=Context(actor,node);Accepted(c,Cmd(c,CompanionOp::ObservePickup,actor,pickup,node),ctx);Begin(c,actor,companion::Goal::SearchResources,pickup,node,ctx);Finish(c,actor,ctx);}
void Transfer(CampaignCore& c,const std::string& actor,const std::string& target,const std::string& item,int quantity,const std::string& node){auto ctx=Context(actor,node);Together(ctx,node);auto a=Cmd(c,CompanionOp::Transfer,actor,target,node);a.item=item;a.quantity=quantity;Accepted(c,a,ctx);}
void ProfilesAndTransfers(){
  for(const auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}){
    CampaignCore c(mode,17);const auto profile=partydata::For(mode);int index=0;
    for(const auto person:{companion::Character::Harley,companion::Character::Bartholomeu,companion::Character::Jack}){const auto b=c.CompanionRuntime(person);Check(b.inventory.capacitySlots==profile.capacities[index]&&b.inventory.contents==profile.loadouts[index],"Authored distinct inventories are initialized");Check(b.survival.injury==survival::Injury::None&&!b.task,"Companions begin healthy without phantom tasks");++index;}
    auto ctx=Context("joe","F4_START");Together(ctx,"F4_START");auto a=Cmd(c,CompanionOp::Transfer,"joe","harley","F4_START");a.item="magazine";a.quantity=1;
    auto absent=ctx;absent.actors.erase("harley");Reject(c,a,absent);auto distant=ctx;distant.actors["harley"].node="F3_ICU";Reject(c,a,distant);
    const auto before=c.Inventory();const auto trust=c.Companion(companion::Character::Harley).traits.trust;Accepted(c,a,ctx);
    Check(Quantity(c.Inventory(),"magazine")==Quantity(before,"magazine")-1&&Quantity(Body(c,"harley").inventory,"magazine")==1,"Transfer conserves actual ammunition");
    Check(c.Companion(companion::Character::Harley).traits.ammunition==1&&c.Companion(companion::Character::Harley).traits.trust==std::min(100.0,trust+20),"GDD ammunition influence matches individual inventory");
    const auto bytes=Bytes(c);CampaignCore loaded(mode,200);Restore(loaded,bytes);Check(loaded.ApplyCompanion(a,ctx).status==Status::AlreadyApplied,"Loaded transfer never repeats item or influence");
    auto conflicting=a;conflicting.quantity=2;Reject(loaded,conflicting,ctx,Status::Conflict);
    auto cross=Action{a.eventId,ActionKind::ShareAmmo,"harley","F4_START",c.Now(),0,true};Check(c.Apply(cross).status==Status::Conflict,"World and companion APIs share one occurrence namespace");
    Transfer(c,"harley","bartholomeu","magazine",1,"F4_START");Check(c.Companion(companion::Character::Harley).traits.ammunition==0&&c.Companion(companion::Character::Bartholomeu).traits.ammunition==1,"Ammo proxy follows companion-to-companion transfer");
    Transfer(c,"bartholomeu","joe","magazine",1,"F4_START");Check(c.Inventory().contents==before.contents,"Roundtrip does not manufacture resources");
    c.SetPaused(true);auto next=Cmd(c,CompanionOp::Transfer,"joe","harley","F4_START");next.item="magazine";next.quantity=1;Reject(c,next,ctx,Status::Paused);Check(c.Tick(10,survival::Activity::Rest).status==Status::Paused,"Pause freezes every health state");c.SetPaused(false);
    const auto saved=Bytes(c);Check(c.Tick(1,survival::Activity::Rest,{{"thomas",survival::Activity::Walk}}).status==Status::InvalidAction&&Bytes(c)==saved,"Unknown companion activity cannot advance clock");
  }
  CampaignCore crowded(rules::Difficulty::Punitive);Transfer(crowded,"joe","bartholomeu","rifle",1,"F4_START");Transfer(crowded,"joe","bartholomeu","magazine",1,"F4_START");Joe(crowded,ActionKind::Collect,"P_MAGAZINE","F2_SUPPLY");auto ctx=Context("joe","F4_START");Together(ctx,"F4_START");auto a=Cmd(crowded,CompanionOp::Transfer,"joe","bartholomeu","F4_START");a.item="magazine";a.quantity=1;Reject(crowded,a,ctx);
}
void TasksAndReservations(){
  CampaignCore c(rules::Difficulty::Hard);auto ctx=Context("harley","F1_PHARMACY");Together(ctx,"F1_PHARMACY");
  Reject(c,Cmd(c,CompanionOp::BeginTask,"harley","P_TREATMENT","F1_PHARMACY",1),ctx);
  Accepted(c,Cmd(c,CompanionOp::ObservePickup,"harley","P_TREATMENT","F1_PHARMACY"),ctx);
  Accepted(c,Cmd(c,CompanionOp::ObservePickup,"jack","P_TREATMENT","F1_PHARMACY"),ctx);
  Reject(c,Cmd(c,CompanionOp::ObserveMissingPickup,"jack","P_TREATMENT","F1_PHARMACY"),ctx);
  Begin(c,"harley",companion::Goal::SearchResources,"P_TREATMENT","F1_PHARMACY",ctx);
  Check(Quantity(Body(c,"harley").inventory,"treatment")==0&&c.World().objects.at("P_TREATMENT").present,"Planning reserves without collecting");
  Reject(c,Cmd(c,CompanionOp::BeginTask,"jack","P_TREATMENT","F1_PHARMACY",1),ctx);
  const auto before=Bytes(c);Check(c.Apply({Id(),ActionKind::Collect,"P_TREATMENT","F1_PHARMACY",c.Now(),0,true}).status==Status::MissingRequirement&&Bytes(c)==before,"Joe cannot consume another actor's reserved pickup");
  auto early=Completion(c,"harley");Reject(c,early,ctx);
  auto arrival=Cmd(c,CompanionOp::Arrive,"harley","P_TREATMENT","F1_PHARMACY");arrival.taskSerial=Body(c,"harley").serial;auto noMotion=ctx;noMotion.movementCompleted=false;Reject(c,arrival,noMotion);auto noPath=ctx;noPath.reachableNodes.clear();Reject(c,arrival,noPath);
  const auto travelling=Bytes(c);CampaignCore resumed(rules::Difficulty::Hard,88);Restore(resumed,travelling);Arrive(resumed,"harley",ctx);
  const auto ready=Bytes(resumed);CampaignCore loaded(rules::Difficulty::Hard);Restore(loaded,ready);Reject(loaded,Completion(loaded,"harley"),ctx);Tick(loaded,1);
  auto wrongGoal=Completion(loaded,"harley");wrongGoal.argument=0;Reject(loaded,wrongGoal,ctx);auto missingAction=ctx;missingAction.actionCompleted=false;Reject(loaded,Completion(loaded,"harley"),missingAction);
  auto complete=Completion(loaded,"harley");Accepted(loaded,complete,ctx);Check(Quantity(Body(loaded,"harley").inventory,"treatment")==1&&!loaded.World().objects.at("P_TREATMENT").present&&!Body(loaded,"harley").knownPickups.count("P_TREATMENT"),"Confirmed collection commits both physical object and own bag");Check(loaded.ApplyCompanion(complete,ctx).status==Status::AlreadyApplied,"Collection retry emits nothing twice");
  Begin(loaded,"jack",companion::Goal::SearchResources,"P_TREATMENT","F1_PHARMACY",ctx);Arrive(loaded,"jack",ctx);Tick(loaded,1);Reject(loaded,Completion(loaded,"jack"),ctx);
  auto release=Cmd(loaded,CompanionOp::Cancel,"jack","P_TREATMENT","F1_PHARMACY");release.taskSerial=Body(loaded,"jack").serial;Accepted(loaded,release,ctx);
  Accepted(loaded,Cmd(loaded,CompanionOp::ObserveMissingPickup,"jack","P_TREATMENT","F1_PHARMACY"),ctx);Check(!Body(loaded,"jack").knownPickups.count("P_TREATMENT"),"Observed absence removes a stale search target");
  Transfer(loaded,"harley","joe","treatment",1,"F1_PHARMACY");Joe(loaded,ActionKind::Drop,"P_TREATMENT","F1_PHARMACY");Accepted(loaded,Cmd(loaded,CompanionOp::ObservePickup,"jack","P_TREATMENT","F1_PHARMACY"),ctx);Check(Body(loaded,"jack").knownPickups.count("P_TREATMENT"),"A returned resource can be discovered again without granting remote knowledge");Restore(loaded,Bytes(loaded));
  // Jack learns disappearance only after physically inspecting the empty spot.
  // Cancel a different pending item without spending or confusing old callbacks.
  auto supply=Context("jack","F2_SUPPLY");Accepted(loaded,Cmd(loaded,CompanionOp::ObservePickup,"jack","P_MAGAZINE","F2_SUPPLY"),supply);Begin(loaded,"jack",companion::Goal::SearchResources,"P_MAGAZINE","F2_SUPPLY",supply);
  auto stale=Completion(loaded,"jack");auto cancel=Cmd(loaded,CompanionOp::Cancel,"jack","P_MAGAZINE","F2_SUPPLY");cancel.taskSerial=Body(loaded,"jack").serial;Accepted(loaded,cancel,supply);Check(loaded.World().objects.at("P_MAGAZINE").present,"Cancellation leaves resource in world");Begin(loaded,"jack",companion::Goal::SearchResources,"P_MAGAZINE","F2_SUPPLY",supply);Reject(loaded,stale,supply);Arrive(loaded,"jack",supply);Tick(loaded,1);Accepted(loaded,Completion(loaded,"jack"),supply);
}
void HideDamageAndAid(){
  auto definition=data::Hospital();for(auto& p:definition.pickups)if(p.id=="P_TREATMENT")p.quantity=2;
  CampaignCore c(rules::Difficulty::Hard,definition);auto ctx=Context("bartholomeu","F4_STORAGE");Together(ctx,"F4_STORAGE");ctx.inDanger=true;
  Accepted(c,Cmd(c,CompanionOp::ObserveHide,"bartholomeu","H_F4_SHEETS","F4_STORAGE"),ctx);Begin(c,"bartholomeu",companion::Goal::StayHidden,"H_F4_SHEETS","F4_STORAGE",ctx);Finish(c,"bartholomeu",ctx);Check(Body(c,"bartholomeu").hidingPlace=="H_F4_SHEETS","Hiding can conclude under danger when physically verified");
  const auto before=Bytes(c);Check(c.Apply({Id(),ActionKind::EnterHide,"H_F4_SHEETS","F4_STORAGE",c.Now(),0,true}).status==Status::MissingRequirement&&Bytes(c)==before,"Joe cannot occupy a companion's hide");Check(c.Tick(2,survival::Activity::Rest,{{"bartholomeu",survival::Activity::Sprint}}).status==Status::InvalidAction,"Hidden actor cannot sprint");
  Restore(c,Bytes(c));Accepted(c,Cmd(c,CompanionOp::LeaveHide,"bartholomeu","H_F4_SHEETS","F4_STORAGE"),ctx);
  Accepted(c,Cmd(c,CompanionOp::Injury,"bartholomeu","bartholomeu","F4_STORAGE",3),ctx);Tick(c,20);Check(Body(c,"bartholomeu").survival.bleeding&&Body(c,"bartholomeu").survival.bloodLoss>0,"Companion graded injury bleeds independently");Check(c.Survival().bloodLoss==0,"Companion wound never becomes Joe's wound");
  Joe(c,ActionKind::Collect,"P_TREATMENT","F1_PHARMACY");
  auto med=Cmd(c,CompanionOp::Stabilize,"joe","bartholomeu","F4_STORAGE");med.item="treatment";Accepted(c,med,ctx);const double blood=Body(c,"bartholomeu").survival.bloodLoss;Tick(c,5);Check(Body(c,"bartholomeu").survival.bloodLoss==blood&&Body(c,"bartholomeu").survival.injury==survival::Injury::Severe,"Stabilize stops bleeding without erasing injury");
  med=Cmd(c,CompanionOp::Treat,"joe","bartholomeu","F4_STORAGE");med.item="treatment";Accepted(c,med,ctx);Check(Body(c,"bartholomeu").survival.injury==survival::Injury::Moderate&&c.Companion(companion::Character::Bartholomeu).traits.injured,"Treatment improves one level and preserves injury proxy");
  CampaignCore healer(rules::Difficulty::Easy);Joe(healer,ActionKind::Injury,"joe","F4_START",1);auto help=Context("harley","F4_START");Together(help,"F4_START");help.joeNeedsHelpObserved=true;
  auto blind=help;blind.actors.erase("joe");Reject(healer,Cmd(healer,CompanionOp::BeginTask,"harley","joe","F4_START",2),blind);Begin(healer,"harley",companion::Goal::HelpJoe,"joe","F4_START",help);
  const auto item=Quantity(Body(healer,"harley").inventory,"treatment");Accepted(healer,Cmd(healer,CompanionOp::Injury,"harley","harley","F4_START",1),help);Check(!Body(healer,"harley").task&&Body(healer,"harley").lastStop==TaskStop::Injured&&Quantity(Body(healer,"harley").inventory,"treatment")==item,"Injury cancels help without burning medicine");
  auto treatment=Cmd(healer,CompanionOp::Treat,"joe","harley","F4_START");treatment.item="treatment";Accepted(healer,treatment,help);Begin(healer,"harley",companion::Goal::HelpJoe,"joe","F4_START",help);Finish(healer,"harley",help);Check(healer.Survival().injury==survival::Injury::None&&Quantity(Body(healer,"harley").inventory,"treatment")==0,"Help spends companion medicine on Joe only when completed");
  CampaignCore fatal(rules::Difficulty::Hard);auto hurt=Context("jack","F4_START");Accepted(fatal,Cmd(fatal,CompanionOp::Injury,"jack","jack","F4_START",3),hurt);Tick(fatal,300);Check(fatal.Now()==300&&fatal.Ending()==rules::Ending::None&&fatal.Narrative().outcomes.at("jack").dead,"Individual death never ends healthy Joe's run");Check(Body(fatal,"jack").survival.bloodLoss==100,"Long tick caps individual blood at exact death");Restore(fatal,Bytes(fatal));Reject(fatal,Cmd(fatal,CompanionOp::ObserveRoute,"jack","tunnel","B1_TUNNEL"),Context("jack","B1_TUNNEL"));
}
void CollapseEndAndBodyOccupancy(){
  CampaignCore c(rules::Difficulty::Easy);auto ctx=Context("jack","F4_START");Together(ctx,"F4_START");Accepted(c,Cmd(c,CompanionOp::Injury,"jack","jack","F4_START",3),ctx);Tick(c,230);
  Check(Body(c,"jack").survival.collapsed&&!c.Narrative().outcomes.at("jack").dead,"Collapse remains rescuable before confirmed death");
  auto medicine=Cmd(c,CompanionOp::Treat,"joe","jack","F4_START");medicine.item="treatment";Accepted(c,medicine,ctx);Check(!Body(c,"jack").survival.collapsed&&!Body(c,"jack").survival.bleeding,"Another actor's aid can rescue a collapsed companion");Restore(c,Bytes(c));
  auto storage=Context("harley","F4_STORAGE");Accepted(c,Cmd(c,CompanionOp::ObserveHide,"harley","H_F4_SHEETS","F4_STORAGE"),storage);Begin(c,"harley",companion::Goal::StayHidden,"H_F4_SHEETS","F4_STORAGE",storage);Finish(c,"harley",storage);Accepted(c,Cmd(c,CompanionOp::Outcome,"harley","outcome","F4_STORAGE",1),storage);Check(Body(c,"harley").survival.dead&&Body(c,"harley").hidingPlace=="H_F4_SHEETS"&&c.World().objects.at("H_F4_SHEETS").stateId=="occupied","Death does not silently remove a body's physical occupancy");Restore(c,Bytes(c));
  CampaignCore end(rules::Difficulty::Normal);auto hall=Context("bartholomeu","F4_STORAGE");Accepted(end,Cmd(end,CompanionOp::ObserveHide,"bartholomeu","H_F4_SHEETS","F4_STORAGE"),hall);Begin(end,"bartholomeu",companion::Goal::StayHidden,"H_F4_SHEETS","F4_STORAGE",hall);
  Joe(end,ActionKind::Trigger,"find_tunnel","B1_TUNNEL");Joe(end,ActionKind::Trigger,"tunnel_exit","X_TUNNEL");Check(!Body(end,"bartholomeu").task&&Body(end,"bartholomeu").lastStop==TaskStop::CampaignEnded,"Joe's confirmed ending interrupts pending companion action without completing it");Check(!end.Narrative().outcomes.at("harley").together&&!end.Narrative().outcomes.at("harley").dead,"Unseen companion fate stays unconfirmed after Joe escapes");auto escaped=Context("harley","X_TUNNEL");Together(escaped,"X_TUNNEL");Accepted(end,Cmd(end,CompanionOp::Outcome,"harley","outcome","X_TUNNEL",2),escaped);Check(end.Narrative().outcomes.at("harley").together,"Physical proof records companion escape together separately");Restore(end,Bytes(end));
  CampaignCore sprint(rules::Difficulty::Normal);Check(sprint.Tick(2,survival::Activity::Rest,{{"jack",survival::Activity::Sprint}}).status==Status::Applied,"Own activity advances own stamina");Check(Body(sprint,"jack").survival.stamina<100&&sprint.Survival().stamina==100&&Body(sprint,"harley").survival.stamina==100,"Companion stamina is independent of Joe and other actors");Restore(sprint,Bytes(sprint));
}
void KnowledgeOutcomesDevices(){
  CampaignCore c(rules::Difficulty::Hard);auto ctx=Context("jack","B1_TUNNEL");Together(ctx,"B1_TUNNEL");ctx.reachableNodes.insert("X_TUNNEL");Accepted(c,Cmd(c,CompanionOp::ObserveRoute,"jack","tunnel","B1_TUNNEL"),ctx);Check(c.JoeKnownRoutes().empty()&&!c.Companion(companion::Character::Harley).knownRoutes.count("tunnel"),"Personal discovery does not grant others knowledge");Begin(c,"jack",companion::Goal::ShareRoute,"tunnel","B1_TUNNEL",ctx);Finish(c,"jack",ctx);Check(c.JoeKnownRoutes().count("tunnel")&&c.Narrative().inspectedDocuments.empty(),"Actual sharing informs Joe without inventing inspected documents");Begin(c,"jack",companion::Goal::EscapeAlone,"tunnel","B1_TUNNEL",ctx);auto exit=Context("jack","X_TUNNEL");Finish(c,"jack",exit);Check(c.Narrative().outcomes.at("jack").escapedAlone&&c.Ending()==rules::Ending::None&&!c.Narrative().outcomes.at("jack").dead,"Individual escape cannot force Joe's victory or infer death");Restore(c,Bytes(c));
  CampaignCore easy(rules::Difficulty::Easy);Accepted(easy,Cmd(easy,CompanionOp::ObserveRoute,"jack","tunnel","B1_TUNNEL"),ctx);Reject(easy,Cmd(easy,CompanionOp::BeginTask,"jack","tunnel","B1_TUNNEL",4),ctx);
  CampaignCore capture(rules::Difficulty::Normal);auto scene=Context("harley","F3_ICU");auto outcome=Cmd(capture,CompanionOp::Outcome,"harley","outcome","F3_ICU",0);auto unproven=scene;unproven.actionCompleted=false;Reject(capture,outcome,unproven);Accepted(capture,outcome,scene);Check(capture.Narrative().outcomes.at("harley").captured&&!capture.Narrative().outcomes.at("harley").dead,"Actual capture remains distinct from death");Restore(capture,Bytes(capture));
  CampaignCore devices(rules::Difficulty::Hard);Collect(devices,"jack","P_RADIO","F1_RADIO");Collect(devices,"jack","P_BATTERY","F2_SERVER");auto radio=Context("jack","F1_RADIO");Accepted(devices,Cmd(devices,CompanionOp::Operate,"jack","install_battery","F1_RADIO",static_cast<int>(ActionKind::Use)),radio);Check(Quantity(Body(devices,"jack").inventory,"battery")==0&&Quantity(Body(devices,"jack").inventory,"radio")==1,"Device operation spends actor's resource and keeps reusable tool");Reject(devices,Cmd(devices,CompanionOp::Operate,"jack","safe_code_accepted","F2_SAFE",static_cast<int>(ActionKind::Use)),Context("jack","F2_SAFE"),Status::InvalidAction);
  auto power=Cmd(devices,CompanionOp::Operate,"jack","C_SERVICE","B1_GENERATOR",static_cast<int>(ActionKind::Circuit));power.quantity=1;const auto signal=devices.ApplyCompanion(power,Context("jack","B1_GENERATOR"));Check(signal.status==Status::Applied&&signal.worldSignal,"Own circuit operation reports physical sound once");Check(!devices.Hunter().lastEstimate,"Device sound never bypasses acoustic perception");Bytes(devices);
}
void BetrayalAndRng(){
  for(const auto mode:{rules::Difficulty::Hard,rules::Difficulty::Punitive}){
    CampaignCore c(mode,121);const std::string actor=mode==rules::Difficulty::Hard?"jack":"bartholomeu";auto ctx=Context("joe","F4_START");Together(ctx,"F4_START");Accepted(c,Cmd(c,CompanionOp::Abandon,"joe",actor,"F4_START"),ctx);ctx.hunterPerceived=true;ctx.joeWitnessedAction=true;ctx.actualSourceMeters={7,8,9};Begin(c,actor,companion::Goal::UseJoeAsDistraction,"joe","F4_START",ctx);Arrive(c,actor,ctx);Tick(c,1);auto gone=ctx;gone.hunterPerceived=false;Reject(c,Completion(c,actor),gone);const auto result=c.ApplyCompanion(Completion(c,actor),ctx);Check(result.status==Status::Applied&&result.actorSignal&&result.actorSignal->actualSourceMeters.x==7,"Executed distraction reports companion's own source");Check(!c.Hunter().lastEstimate&&c.Ending()==rules::Ending::None&&!c.Narrative().outcomes.at(actor).dead,"Betrayal never magically reveals, captures or kills anyone");Check(bool(c.Narrative().durableFacts.count("barth_betrayal_executed"))==(actor=="bartholomeu"),"Joe witnessed actual betrayal, not just chosen goal");Restore(c,Bytes(c));
  }
  CampaignCore c(rules::Difficulty::Normal,42);auto ctx=Context("harley","F4_STORAGE");Accepted(c,Cmd(c,CompanionOp::ObserveHide,"harley","H_F4_SHEETS","F4_STORAGE"),ctx);auto atSupply=Context("harley","F2_SUPPLY");Accepted(c,Cmd(c,CompanionOp::ObservePickup,"harley","P_MAGAZINE","F2_SUPPLY"),atSupply);ctx.reachableNodes.insert("F2_SUPPLY");const auto bytes=Bytes(c);CampaignCore loaded(rules::Difficulty::Normal,999);Restore(loaded,bytes);auto choose=Cmd(c,CompanionOp::ChooseTask,"harley","auto","F4_STORAGE");Accepted(c,choose,ctx);Accepted(loaded,choose,ctx);Check(Bytes(c)==Bytes(loaded),"Filtered stochastic policy resumes exactly from persisted RNG");
  auto cancel=Cmd(c,CompanionOp::Cancel,"harley",Body(c,"harley").task->target,"F4_STORAGE");cancel.taskSerial=Body(c,"harley").serial;Accepted(c,cancel,ctx);
  auto noPath=ctx;noPath.reachableNodes.clear();Check(c.CompanionOffers("harley","F4_STORAGE",noPath).empty(),"No verified destination means no phantom fallback action");
}
void FractionalTaskDeadline(){
  CampaignCore c(rules::Difficulty::Hard);Tick(c,8191.999999999997);auto ctx=Context("harley","F4_STORAGE");Accepted(c,Cmd(c,CompanionOp::ObserveHide,"harley","H_F4_SHEETS","F4_STORAGE"),ctx);Begin(c,"harley",companion::Goal::StayHidden,"H_F4_SHEETS","F4_STORAGE",ctx);Finish(c,"harley",ctx);Check(Body(c,"harley").hidingPlace=="H_F4_SHEETS","Exact interaction deadline tolerates shared clock floating-point representation");
}
void Integrity(){
  CampaignCore c(rules::Difficulty::Normal);auto ctx=Context("harley","F4_STORAGE");Accepted(c,Cmd(c,CompanionOp::ObserveHide,"harley","H_F4_SHEETS","F4_STORAGE"),ctx);Begin(c,"harley",companion::Goal::StayHidden,"H_F4_SHEETS","F4_STORAGE",ctx);const auto clean=Bytes(c);
  const auto reject=[&](std::string bad){const auto checksum=bad.rfind("CHECKSUM ");bad.resize(checksum);bad+="CHECKSUM "+detail::Hash(bad)+'\n';const auto before=Bytes(c);std::istringstream in(bad);Check(c.Load(in)!=save::Result::Ok&&Bytes(c)==before,"Tampered individual snapshot cannot partially load");};
  auto bad=clean;auto index=bad.find("INTENT 1 0");bad.replace(index,10,"INTENT 7 0");reject(bad);
  bad=clean;index=bad.find("HIDES 1\n\"H_F4_SHEETS\"");bad.replace(index,24,"HIDES 1\n\"H_F3_ISOLATION\"");reject(bad);
  bad=clean;index=bad.find("BODY \"harley\"");const auto serial=bad.find(" 1 0\n",index);bad.replace(serial,5," 9 0\n");reject(bad);
  bad=clean;index=bad.find("JOE_ROUTES 0");bad.replace(index,12,"JOE_ROUTES 1\n\"tunnel\"");reject(bad);
  bad=clean;index=bad.find("HEALTH 0 100 0 0 0 0 0",bad.find("BODY \"harley\""));bad.replace(index,22,"HEALTH 1 100 0 0 0 0 0");reject(bad);
  CampaignCore captured(rules::Difficulty::Normal);auto at=Context("jack","F4_START");Accepted(captured,Cmd(captured,CompanionOp::Outcome,"jack","outcome","F4_START",0),at);auto unsupportedFate=Bytes(captured);
  const auto receipt=unsupportedFate.find("RECEIPT ");const auto end=unsupportedFate.find('\n',receipt);unsupportedFate.erase(receipt,end-receipt+1);unsupportedFate.replace(unsupportedFate.find("RECEIPTS 1"),std::string("RECEIPTS 1").size(),"RECEIPTS 0");reject(unsupportedFate);
  auto old=clean;old.replace(0,std::string("THE_THUG_RUNTIME_CAMPAIGN 6").size(),"THE_THUG_RUNTIME_CAMPAIGN 2");std::istringstream legacy(old);Check(c.Load(legacy)==save::Result::UnsupportedVersion,"Old runtime never silently loses individual state");
}
}
int main(){try{ProfilesAndTransfers();TasksAndReservations();HideDamageAndAid();CollapseEndAndBodyOccupancy();KnowledgeOutcomesDevices();BetrayalAndRng();FractionalTaskDeadline();Integrity();std::cout<<"CampaignCompanions: "<<checks<<" checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<"After "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
