#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "SpecialIO.h"
#include <iostream>
#include <random>
#include <stdexcept>
using namespace thug;
using campaign::Status;
using campaign::SpecialOp;
using campaign::SpecialFate;
using campaign::SpecialPhase;
namespace {
int checks = 0, events = 0;
void Check(bool ok, const char* reason) { ++checks; if (!ok) throw std::runtime_error(reason); }
std::string Bytes(const campaign::CampaignCore& core) {
  std::ostringstream out; Check(core.Save(out) == save::Result::Ok, "Special state has a coherent checkpoint"); return out.str();
}
director::Observation At(const campaign::CampaignCore& c, const std::string& node) {
  director::Observation o; o.difficulty = c.Narrative().difficulty; o.act = c.Narrative().act; o.nowSeconds = c.Now(); o.joeNode = node;
  for (const auto& name : {"thomas","harley","bartholomeu","jack"}) o.companions.emplace(name, director::Actor{node,true});
  return o;
}
campaign::Action Action(const campaign::CampaignCore& c, campaign::ActionKind kind, const std::string& target, const std::string& node) {
  return {"special-test-"+std::to_string(++events),kind,target,node,c.Now(),0,true};
}
void Complete(campaign::CampaignCore& c, director::Observation o) {
  while (const auto cue = c.CurrentPresentation(o)) {
    auto a = Action(c,campaign::ActionKind::SceneCue,cue->owner,o.joeNode);
    a.presentationSerial = cue->serial; a.argument = cue->index; a.cueId = cue->cue.id;
    Check(c.ApplyNarrative(a,o).status == Status::Applied,"Actual scene ACK commits once");
    o.act = c.Narrative().act;
  }
}
void Opening(campaign::CampaignCore& c, int act = 5) {
  for (const auto& [id,node] : std::map<std::string,std::string>{{"N01","F4_START"},{"N02","F4_OBSERVATION"},{"N03","F4_START"}}) {
    auto o = At(c,node); Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,id,node),o).status == Status::Applied,"Opening begins"); Complete(c,o);
  }
  auto a = Action(c,campaign::ActionKind::AdvanceAct,"act","F4_START"); a.argument = act;
  Check(c.ApplyNarrative(a,At(c,a.node)).status == Status::Applied,"Authored late milestone exists");
}
campaign::SpecialCommand Command(const campaign::CampaignCore& c, SpecialOp kind, const std::string& target, const std::string& node) {
  campaign::SpecialCommand a; a.eventId = "rare-"+std::to_string(++events); a.kind = kind; a.target = target; a.node = node; a.atSeconds = c.Now(); return a;
}
campaign::SpecialContext Context(const std::string& node) {
  campaign::SpecialContext c; c.actualJoeNode = c.actualHunterNode = node;
  c.physicallyVerified = c.actionCompleted = c.movementCompleted = c.warningPresented = true;
  c.hunterIncapacitatedVerified = c.joeSurvivedCollapse = c.fatesWitnessed = c.appearancePerceived = true;
  for (const auto& name : {"harley","bartholomeu","jack"}) c.participants.emplace(name,director::Actor{node,true});
  return c;
}
void Apply(campaign::CampaignCore& c, campaign::SpecialCommand a) { Check(c.ApplySpecial(a,Context(a.node)) == Status::Applied,"Verified special transition applies"); }
void Rejected(campaign::CampaignCore& c, const campaign::SpecialCommand& a, const campaign::SpecialContext& context, Status expected) {
  const auto before = Bytes(c); Check(c.ApplySpecial(a,context) == expected,"Rejected special reports expected status"); Check(Bytes(c) == before,"Rejected special changes no state/resources/receipt");
}
void Resume(campaign::CampaignCore& c) {
  const auto bytes = Bytes(c); campaign::CampaignCore loaded(c.Narrative().difficulty,9999); std::istringstream in(bytes);
  Check(loaded.Load(in) == save::Result::Ok && Bytes(loaded) == bytes,"Special checkpoint resumes byte-exactly with a different constructor seed"); c = std::move(loaded);
}
void Ammo(campaign::CampaignCore& c, const std::string& name, const std::string& node) {
  campaign::CompanionCommand a; a.eventId = "ammo-"+std::to_string(++events); a.kind = campaign::CompanionOp::Transfer;
  a.actor = "joe"; a.target = name; a.node = node; a.atSeconds = c.Now(); a.item = "magazine"; a.quantity = 1; a.physicallyVerified = true;
  campaign::CompanionContext context; context.actionCompleted = context.joeWitnessedAction = true;
  for (const auto& actor : {"joe","harley","bartholomeu","jack"}) context.actors.emplace(actor,director::Actor{node,true});
  Check(c.ApplyCompanion(a,context).status == Status::Applied,"Each participant receives a real conserved magazine");
}
void PrepareConfrontation(campaign::CampaignCore& c) {
  const auto d = *campaign::data::Hospital().finale;
  for (const auto& actor : {"harley","bartholomeu"}) { Ammo(c,actor,d.confrontationNode); Apply(c,Command(c,SpecialOp::Recruit,actor,d.confrontationNode)); }
}
void Confrontation() {
  const auto d = *campaign::data::Hospital().finale;
  campaign::CampaignCore c(rules::Difficulty::Easy); Opening(c);
  auto start = Command(c,SpecialOp::StartConfrontation,"confrontation",d.confrontationNode);
  Rejected(c,start,Context(start.node),Status::MissingRequirement);
  PrepareConfrontation(c); const auto before = c.Inventory().contents.at("magazine");
  auto missing = Context(start.node); missing.participants.erase("bartholomeu"); Rejected(c,start,missing,Status::MissingRequirement);
  missing = Context(start.node); missing.warningPresented = false; Rejected(c,start,missing,Status::MissingRequirement);
  Apply(c,start); Check(c.Inventory().contents.at("magazine") == before-1,"Committed confrontation consumes Joe's own supply");
  Check(c.CompanionRuntime(companion::Character::Harley).inventory.contents.empty() || !c.CompanionRuntime(companion::Character::Harley).inventory.contents.count("magazine"),"Participant ammunition is consumed separately");
  const auto committed = Bytes(c); Check(c.ApplySpecial(start,Context(start.node)) == Status::AlreadyApplied && Bytes(c) == committed,"Retry cannot consume ammunition twice");
  Resume(c); Check(c.Tick(100,survival::Activity::Rest).advancedSeconds == d.confrontationSeconds,"Long delta stops at physical confrontation boundary");
  Check(c.Tick(1,survival::Activity::Rest).status == Status::AwaitingPhysics,"Intent alone does not incapacitate Hunter");
  auto resolve = Command(c,SpecialOp::ResolveConfrontation,"confrontation",d.confrontationNode); resolve.argument = 1;
  missing = Context(resolve.node); missing.hunterIncapacitatedVerified = false; Rejected(c,resolve,missing,Status::MissingRequirement);
  missing = Context(resolve.node); missing.actualHunterNode = "F4_START"; Rejected(c,resolve,missing,Status::MissingRequirement);
  resolve.fates["harley"] = SpecialFate::Injured; Apply(c,resolve);
  Check(c.Ending() == rules::Ending::None && c.Survival().injury == survival::Injury::Moderate,"Interruption opens an opportunity and inflicts authored cost without victory");
  campaign::HunterContext hc; hc.actualHunterNode = d.confrontationNode; hc.reachableNodes.insert(d.confrontationNode);
  Check(c.HunterOffers(hc).empty(),"Hunter cannot select tactics during rare interruption window");
  Resume(c); auto escape = Command(c,SpecialOp::ConfrontationEscape,"confrontation",d.confrontationExit); escape.fates["harley"] = SpecialFate::Escaped;
  missing = Context(escape.node); missing.movementCompleted = false; Rejected(c,escape,missing,Status::MissingRequirement);
  auto absent = escape; absent.fates["jack"] = SpecialFate::Escaped; Rejected(c,absent,Context(absent.node),Status::MissingRequirement);
  Apply(c,escape); Check(c.Ending() == rules::Ending::CoordinatedEscape && c.Narrative().joeEscaped,"Verified crossing has its own campaign result");
  Check(c.Narrative().outcomes.at("harley").together && !c.Narrative().outcomes.at("bartholomeu").together,"Helping Joe does not automatically escape every companion");
  auto o = At(c,d.confrontationExit); Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,"N33",o.joeNode),o).status == Status::Applied,"Rare terminal scene now activates from real result");
  bool harley = false;
  while (const auto p = c.CurrentPresentation(o)) {
    Check(p->cue.speaker != "bartholomeu" && p->cue.speaker != "jack","Absent companions never speak in rare ending"); harley = harley || p->cue.speaker == "harley";
    auto ack = Action(c,campaign::ActionKind::SceneCue,p->owner,o.joeNode); ack.presentationSerial = p->serial; ack.argument = p->index; ack.cueId = p->cue.id;
    Check(c.ApplyNarrative(ack,o).status == Status::Applied,"Rare cue confirms after campaign end"); Resume(c);
  }
  Check(harley && c.Narrative().seenScenes.count("N33"),"Present survivor has her own ending line");
  Check(!c.EpilogueText().empty() && c.EpilogueText().front().owner == "E_RARE_CONFRONTATION","Rare confrontation has corresponding epilogue");
  campaign::CampaignCore expired(rules::Difficulty::Easy); Opening(expired); PrepareConfrontation(expired);
  Apply(expired,Command(expired,SpecialOp::StartConfrontation,"confrontation",d.confrontationNode)); expired.Tick(d.confrontationSeconds,survival::Activity::Rest);
  resolve = Command(expired,SpecialOp::ResolveConfrontation,"confrontation",d.confrontationNode); resolve.argument = 1; Apply(expired,resolve);
  expired.Tick(d.opportunitySeconds,survival::Activity::Rest);
  Rejected(expired,Command(expired,SpecialOp::ConfrontationEscape,"confrontation",d.confrontationExit),Context(d.confrontationExit),Status::MissingRequirement);
  Check(expired.Ending() == rules::Ending::None,"Missed opportunity does not kill Joe or grant victory");
  campaign::CampaignCore failed(rules::Difficulty::Easy); Opening(failed); PrepareConfrontation(failed);
  Apply(failed,Command(failed,SpecialOp::StartConfrontation,"confrontation",d.confrontationNode)); failed.Tick(d.confrontationSeconds,survival::Activity::Rest);
  resolve = Command(failed,SpecialOp::ResolveConfrontation,"confrontation",d.confrontationNode); resolve.fates["bartholomeu"] = SpecialFate::Dead; Apply(failed,resolve); Resume(failed);
  Check(failed.Ending() == rules::Ending::None && failed.Narrative().outcomes.at("bartholomeu").dead && !failed.Narrative().outcomes.at("harley").dead,"Failure keeps individual physically witnessed consequences");
}
void RescueConcurrency() {
  const auto d=*campaign::data::Hospital().finale;
  campaign::CampaignCore c(rules::Difficulty::Easy); Opening(c); PrepareConfrontation(c);
  campaign::HunterContext hunter; hunter.actualHunterNode=d.confrontationNode;
  hunter.physicallyVerified=hunter.lineOfSightVerified=true;
  campaign::HunterCommand sample; sample.kind=campaign::HunterOp::SeeActor; sample.actor="jack";
  sample.node=d.confrontationNode; sample.atSeconds=c.Now();
  Check(c.SenseHunter(sample,hunter),"Real third participant observed");
  campaign::RescueContext context; context.actualHunterNode=d.confrontationNode;
  context.contactVerified=context.continuousContactVerified=context.actionCompleted=context.openingVerified=true;
  for(const auto& actor:{"joe","harley","bartholomeu","jack"})context.actors.emplace(actor,director::Actor{d.confrontationNode,true});
  const auto command=[&](campaign::RescueOp kind,const std::string& actor,std::uint64_t serial=0) {
    campaign::RescueCommand a; a.eventId="joint-"+std::to_string(++events); a.kind=kind; a.actor=actor;
    a.target="jack"; a.node=d.confrontationNode; a.atSeconds=c.Now(); a.serial=serial; a.physicallyVerified=true; return a;
  };
  Check(c.ApplyRescue(command(campaign::RescueOp::BeginContainment,"hunter"),context)==Status::Applied,"Third participant temporarily contained");
  const auto held=c.RescueRuntime().captures.at("jack").serial;
  c.Tick(campaign::RestrainSeconds,survival::Activity::Rest); sample.atSeconds=c.Now();
  Check(c.SenseHunter(sample,hunter),"Contact verified again at physical restraint completion");
  Check(c.ApplyRescue(command(campaign::RescueOp::FinishContainment,"hunter",held),context)==Status::Applied,"Containment completed before other plan");
  context.actualHunterNode="F4_START";
  Check(c.ApplyRescue(command(campaign::RescueOp::BeginRelease,"harley"),context)==Status::Applied,"Recruitment intention alone does not lock helper");
  const auto helping=c.RescueRuntime().captures.at("jack").release->serial;
  Rejected(c,Command(c,SpecialOp::StartConfrontation,"confrontation",d.confrontationNode),Context(d.confrontationNode),Status::MissingRequirement);
  Check(c.ApplyRescue(command(campaign::RescueOp::CancelRelease,"harley",helping),context)==Status::Applied,"Helper stops before joining another committed action");
  Apply(c,Command(c,SpecialOp::StartConfrontation,"confrontation",d.confrontationNode));
  const auto before=Bytes(c);
  Check(c.RescueOffers("harley",context).empty(),"Committed participant has no simultaneous rescue offer");
  Check(c.ApplyRescue(command(campaign::RescueOp::BeginRelease,"harley"),context)==Status::MissingRequirement && Bytes(c)==before,"Committed participant cannot perform two joint actions"); Resume(c);
  c.Tick(d.confrontationSeconds,survival::Activity::Rest);
  auto resolve=Command(c,SpecialOp::ResolveConfrontation,"confrontation",d.confrontationNode); resolve.argument=1; Apply(c,resolve);
  Check(c.ApplyRescue(command(campaign::RescueOp::BeginRelease,"harley"),context)==Status::MissingRequirement,"Participant remains committed through physical opportunity");
  Resume(c);
}
void PlantAll(campaign::CampaignCore& c) {
  const auto d = *campaign::data::Hospital().finale;
  for (std::size_t i = 0; i < d.sites.size(); ++i) {
    const auto& supply = d.supplies[i]; const auto& site = d.sites[i];
    Apply(c,Command(c,SpecialOp::AcquireSupply,supply.id,supply.node));
    Apply(c,Command(c,SpecialOp::BeginPlant,site.id,site.node)); Resume(c);
    auto finish = Command(c,SpecialOp::CompletePlant,site.id,site.node); finish.serial = c.SpecialRuntime().plant->serial;
    Rejected(c,finish,Context(finish.node),Status::MissingRequirement);
    c.Tick(d.plantSeconds,survival::Activity::Rest); finish.atSeconds = c.Now(); Apply(c,finish); Resume(c);
    Check(!c.Inventory().contents.count("structural_charge"),"Plant completion consumes exactly its own charge");
  }
  const auto& p = d.supplies.back(); Apply(c,Command(c,SpecialOp::AcquireSupply,p.id,p.node));
}
void Destruction() {
  const auto d = *campaign::data::Hospital().finale;
  for (bool survives : {true,false}) {
    campaign::CampaignCore c(rules::Difficulty::Punitive); Opening(c);
    Rejected(c,Command(c,SpecialOp::DetonateCollapse,"collapse",d.detonatorNode),Context(d.detonatorNode),Status::MissingRequirement);
    PlantAll(c); const auto checkpoint = Bytes(c);
    auto detonate = Command(c,SpecialOp::DetonateCollapse,"collapse",d.detonatorNode);
    auto missing = Context(detonate.node); missing.warningPresented = false; Rejected(c,detonate,missing,Status::MissingRequirement);
    Apply(c,detonate); Check(!c.Inventory().contents.count("structural_detonator") && c.Ending() == rules::Ending::None,"Detonation costs the real detonator and still requires survival"); Resume(c);
    const auto block = c.Apply(Action(c,campaign::ActionKind::Trigger,"dawn_confirmed","F4_START")); Check(block.status == Status::AwaitingPhysics,"Collapse cannot be bypassed by an ordinary route event");
    auto resolved = Command(c,SpecialOp::ResolveCollapse,"collapse",d.collapseShelter); resolved.argument = survives ? 1 : 0;
    Rejected(c,resolved,Context(resolved.node),Status::MissingRequirement);
    Check(c.Tick(100,survival::Activity::Rest).advancedSeconds == d.collapseSeconds,"Clock stops at collapse physics boundary");
    Check(c.Tick(1,survival::Activity::Rest).status == Status::AwaitingPhysics,"Collapse does not produce automatic victory");
    resolved.atSeconds = c.Now(); auto context = Context(resolved.node);
    if (survives) { context.joeSurvivedCollapse = false; Rejected(c,resolved,context,Status::MissingRequirement); context.joeSurvivedCollapse = true; }
    else { context.joeSurvivedCollapse = false; context.joeDeathVerified = true; }
    Check(c.ApplySpecial(resolved,context) == Status::Applied,"Physical survival or death resolves collapse"); Resume(c);
    Check(c.Ending() == (survives ? rules::Ending::BitterDestruction : rules::Ending::Died) && !c.Narrative().joeEscaped,"Bitter ending is distinguished from freedom and from death");
    for (const auto& site : d.sites) Check(c.World().objects.at(site.id).stateId == "destroyed","Physical collapse state persists");
    for (const auto& [name,fate] : c.Narrative().outcomes) { (void)name; Check(!fate.dead && !fate.together,"Unwitnessed companions remain unaccounted for instead of invented deaths"); }
    if (survives) {
      Check(c.Survival().injury == survival::Injury::Severe && !c.Survival().bleeding,"Surviving collapse leaves Joe severely injured, stabilized for police recovery");
      Check(c.EpilogueText().front().owner == "E_RARE_DESTRUCTION","Bitter destruction has an authored epilogue");
      auto o = At(c,d.collapseShelter); Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,"N33",o.joeNode),o).status == Status::Applied,"Punitive rare scene is enabled only after actual result");
      while (const auto p = c.CurrentPresentation(o)) { Check(p->cue.speaker.empty() || p->cue.speaker == "joe","Collapse does not invent absent NPC voices");
        auto ack = Action(c,campaign::ActionKind::SceneCue,p->owner,o.joeNode); ack.presentationSerial=p->serial; ack.argument=p->index; ack.cueId=p->cue.id; Check(c.ApplyNarrative(ack,o).status==Status::Applied,"Bitter coda ACK confirms"); }
    }
    campaign::CampaignCore rewind(rules::Difficulty::Punitive); std::istringstream in(checkpoint); Check(rewind.Load(in)==save::Result::Ok && rewind.Ending()==rules::Ending::None,"Checkpoint before trigger restores preparation without future knowledge");
  }
}
void AuthorityAndPlant() {
  const auto world = campaign::data::Hospital(); const auto d = *world.finale;
  for (const auto mode : {rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    campaign::CampaignCore c(mode); Opening(c);
    auto o = At(c,d.confrontationNode); o.facts.insert(mode==rules::Difficulty::Punitive?"rare_destruction":"rare_confrontation");
    Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,"N33",o.joeNode),o).status==Status::MissingRequirement,"Injected observation cannot activate rare finale");
    auto fake = Action(c,campaign::ActionKind::RememberFact,"rare_confrontation",o.joeNode);
    Check(c.ApplyNarrative(fake,o).status==Status::InvalidAction,"Rare outcome fact belongs exclusively to campaign result");
    if (mode!=rules::Difficulty::Punitive) Rejected(c,Command(c,SpecialOp::AcquireSupply,d.supplies[0].id,d.supplies[0].node),Context(d.supplies[0].node),Status::MissingRequirement);
    if (mode!=rules::Difficulty::Easy) Rejected(c,Command(c,SpecialOp::Recruit,"harley",d.confrontationNode),Context(d.confrontationNode),Status::MissingRequirement);
  }
  campaign::CampaignCore c(rules::Difficulty::Punitive); Opening(c); const auto& site=d.sites[0]; const auto& p=d.supplies[0];
  const auto supply = Command(c,SpecialOp::AcquireSupply,p.id,p.node); Apply(c,supply); const auto got=Bytes(c);
  Check(c.ApplySpecial(supply,Context(supply.node))==Status::AlreadyApplied && Bytes(c)==got,"Supply receipt survives retries");
  Rejected(c,Command(c,SpecialOp::AcquireSupply,p.id,p.node),Context(p.node),Status::MissingRequirement);
  Apply(c,Command(c,SpecialOp::BeginPlant,site.id,site.node));
  const auto reserved=Bytes(c);
  campaign::CompanionCommand transfer; transfer.eventId="reserved-transfer"; transfer.kind=campaign::CompanionOp::Transfer;
  transfer.actor="joe"; transfer.target="harley"; transfer.node=site.node; transfer.atSeconds=c.Now(); transfer.item="structural_charge"; transfer.quantity=1; transfer.physicallyVerified=true;
  campaign::CompanionContext cc; cc.actionCompleted=true; cc.actors.emplace("joe",director::Actor{site.node,true}); cc.actors.emplace("harley",director::Actor{site.node,true});
  Check(c.ApplyCompanion(transfer,cc).status==Status::MissingRequirement && Bytes(c)==reserved,"Prepared charge cannot be transferred or double spent");
  auto cancel=Command(c,SpecialOp::CancelPlant,site.id,site.node); cancel.serial=c.SpecialRuntime().plant->serial; Apply(c,cancel);
  Check(c.Inventory().contents.at("structural_charge")==1 && c.SpecialRuntime().planted.empty(),"Cancelling physical preparation releases reservation without consumption"); Resume(c);
}
void Appearances() {
  const auto world = campaign::data::Hospital();
  for (const auto mode : {rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    campaign::CampaignCore c(mode); Opening(c);
    for (const auto& a : world.appearances) {
      auto command = Command(c,SpecialOp::WitnessAppearance,a.id,a.node); auto ctx = Context(a.node);
      const bool allowed=(a.modes&(1u<<static_cast<int>(mode))) && c.SpecialRuntime().appearanceTicket<a.tickets;
      if (!allowed) { Rejected(c,command,ctx,Status::MissingRequirement); continue; }
      auto o=At(c,a.node); o.facts.insert("appearance:"+a.id);
      Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,a.block,a.node),o).status==Status::MissingRequirement,"Caller facts cannot invent an appearance");
      ctx.appearancePerceived=false; Rejected(c,command,ctx,Status::MissingRequirement); ctx.appearancePerceived=true;
      Apply(c,command); Resume(c);
      Check(c.Ending()==rules::Ending::None && c.JoeKnownRoutes().empty(),"Special appearances do not grant routes or victory");
      Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,a.block,a.node),At(c,a.node)).status==Status::Applied,"Witnessed encounter selects native presentation"); Complete(c,At(c,a.node)); Resume(c);
      Rejected(c,Command(c,SpecialOp::WitnessAppearance,a.id,a.node),ctx,Status::MissingRequirement);
    }
  }
  std::uint64_t seed=0; for (;;++seed) { std::mt19937_64 random(seed+7); if (random()%1000==0) break; }
  campaign::CampaignCore flash(rules::Difficulty::Hard,seed); Opening(flash); const auto a=world.appearances.back();
  Apply(flash,Command(flash,SpecialOp::WitnessAppearance,a.id,a.node)); Resume(flash);
  Check(flash.SpecialRuntime().appearanceTicket==0 && flash.Ending()==rules::Ending::None,"Rare Flash reference uses one deterministic saved campaign draw without affecting gameplay");
}
void Corruption() {
  const auto d=*campaign::data::Hospital().finale; campaign::CampaignCore c(rules::Difficulty::Punitive); Opening(c);
  Apply(c,Command(c,SpecialOp::AcquireSupply,d.supplies[0].id,d.supplies[0].node)); Apply(c,Command(c,SpecialOp::BeginPlant,d.sites[0].id,d.sites[0].node));
  const auto valid=Bytes(c);
  const auto reject=[&](std::string bad){ const auto at=bad.rfind("CHECKSUM "); bad.resize(at); bad+="CHECKSUM "+campaign::detail::Hash(bad)+'\n';
    const auto before=Bytes(c); std::istringstream in(bad); Check(c.Load(in)!=save::Result::Ok && Bytes(c)==before,"Semantically forged special checkpoint is rejected without mutation even with recomputed checksum"); };
  auto bad=valid; const auto at=bad.find("PLANTED 0"); Check(at!=std::string::npos,"Fixture contains no completed charge"); bad.replace(at,9,"PLANTED 1 \"RARE_SITE_A\""); reject(bad);
  bad=valid; const auto serial=bad.find("SPECIAL 2 1 "); Check(serial!=std::string::npos,"Fixture has two special receipts"); bad.replace(serial,12,"SPECIAL 3 1 "); reject(bad);
  bad=valid; const auto task=bad.find("PLANT 1\n"); Check(task!=std::string::npos,"Fixture reserves a plant task"); const auto end=bad.find('\n',task+8); bad.erase(task,end-task+1); bad.insert(task,"PLANT 0\n"); reject(bad);
  auto legacy=valid; legacy.replace(0,std::string("THE_THUG_RUNTIME_CAMPAIGN 8").size(),"THE_THUG_RUNTIME_CAMPAIGN 5"); std::istringstream old(legacy);
  Check(c.Load(old)==save::Result::UnsupportedVersion,"Runtime v5 cannot silently lose rare finale state");
}
void ReceiptBudget() {
  const auto world=campaign::data::Hospital(); const auto d=*world.finale;
  campaign::CampaignCore c(rules::Difficulty::Easy); Opening(c); PrepareConfrontation(c);
  auto bytes=Bytes(c); const auto line=bytes.find("RECEIPTS "); const auto end=bytes.find('\n',line);
  std::ostringstream padding; padding<<"RECEIPTS 9997\n";
  const auto circuit=*world.circuitNodes.begin();
  for (std::size_t i=c.AppliedActionCount(); i<9997; ++i) {
    auto action=Action(c,campaign::ActionKind::Circuit,circuit.first,circuit.second);
    padding<<"RECEIPT "<<std::quoted(action.eventId)<<' '<<std::quoted(campaign::detail::Signature(action))<<'\n';
  }
  bytes.replace(line,end-line+1,padding.str()); const auto checksum=bytes.rfind("CHECKSUM "); bytes.resize(checksum); bytes+="CHECKSUM "+campaign::detail::Hash(bytes)+'\n';
  std::istringstream in(bytes); Check(c.Load(in)==save::Result::Ok,"Near-budget ledger fixture remains coherent");
  Apply(c,Command(c,SpecialOp::StartConfrontation,"confrontation",d.confrontationNode));
  const auto a=world.appearances[1]; Rejected(c,Command(c,SpecialOp::WitnessAppearance,a.id,a.node),Context(a.node),Status::LogFull);
  c.Tick(d.confrontationSeconds,survival::Activity::Rest);
  auto resolve=Command(c,SpecialOp::ResolveConfrontation,"confrontation",d.confrontationNode); resolve.argument=1; Apply(c,resolve);
  Rejected(c,Command(c,SpecialOp::WitnessAppearance,a.id,a.node),Context(a.node),Status::LogFull);
  Apply(c,Command(c,SpecialOp::ConfrontationEscape,"confrontation",d.confrontationExit));
  Check(c.AppliedActionCount()==10000 && c.Ending()==rules::Ending::CoordinatedEscape,"Committed rare attempt reserves both physical resolution AND evacuation receipts");
  Resume(c);
}
void ForgedAppearance() {
  campaign::CampaignCore c(rules::Difficulty::Hard); Opening(c);
  auto valid=Bytes(c); auto bad=valid;
  const auto heading=bad.find("\nSTORY ")+1; const auto first=bad.find('\n',heading)+1;
  const auto length=static_cast<std::size_t>(std::stoull(bad.substr(heading+6,first-heading-7)));
  auto story=bad.substr(first,length); const auto facts="FACTS "+std::to_string(c.Narrative().durableFacts.size())+"\n";
  const auto at=story.find(facts); Check(at!=std::string::npos,"Nested fixture contains authored fact set");
  story.replace(at,facts.size(),"FACTS "+std::to_string(c.Narrative().durableFacts.size()+1)+"\nFACT \"appearance:alfred\"\n");
  story.resize(story.rfind("CHECKSUM ")); story+="CHECKSUM "+campaign::detail::Hash(story)+'\n';
  bad.replace(heading,first-heading+length,"STORY "+std::to_string(story.size())+'\n'+story);
  const auto reject=[&](std::string forged){
    forged.resize(forged.rfind("CHECKSUM ")); forged+="CHECKSUM "+campaign::detail::Hash(forged)+'\n';
    const auto before=Bytes(c); std::istringstream in(forged); const auto result=c.Load(in);
    if(result!=save::Result::IncompatibleRoster) throw std::runtime_error("Expected appearance ownership rejection, got save result "+std::to_string(static_cast<int>(result)));
    Check(Bytes(c)==before,"Forged appearance knowledge leaves session untouched");
  };
  reject(bad);
  const auto a=campaign::data::Hospital().appearances[1]; auto witness=Command(c,SpecialOp::WitnessAppearance,a.id,a.node); Apply(c,witness);
  auto o=At(c,a.node); Check(c.ApplyNarrative(Action(c,campaign::ActionKind::BeginScene,a.block,a.node),o).status==Status::Applied,"Real appearance fixture presents its line"); Complete(c,o);
  bad=Bytes(c); const auto serial=bad.find("SPECIAL 1 0 "); Check(serial!=std::string::npos,"Fixture has one witnessed appearance"); bad.replace(serial,12,"SPECIAL 0 0 ");
  const std::string encountered="APPEARANCES 1 \"alfred\""; const auto encounter=bad.find(encountered); Check(encounter!=std::string::npos,"Encounter was durably recorded"); bad.replace(encounter,encountered.size(),"APPEARANCES 0");
  const auto receipt=bad.find("RECEIPT \""+witness.eventId+"\""); Check(receipt!=std::string::npos,"Fixture has special receipt"); const auto end=bad.find('\n',receipt); bad.erase(receipt,end-receipt+1);
  const auto count="RECEIPTS "+std::to_string(c.AppliedActionCount()); const auto countAt=bad.find(count); bad.replace(countAt,count.size(),"RECEIPTS "+std::to_string(c.AppliedActionCount()-1));
  reject(bad);
}
}
int main() { try { AuthorityAndPlant(); Confrontation(); RescueConcurrency(); Destruction(); Appearances(); Corruption(); ReceiptBudget(); ForgedAppearance(); std::cout<<"CampaignSpecial: "<<checks<<" checks passed\n"; }
  catch(const std::exception& e) { std::cerr<<"After "<<checks<<" checks: "<<e.what()<<'\n'; return 1; } }
