#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "CompanionData.generated.h"
#include "CombatIO.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace thug;
using campaign::CombatOp;
using campaign::Status;
namespace {
int checks = 0, occurrence = 0;
void Check(bool ok, const char* reason) { ++checks; if (!ok) throw std::runtime_error(reason); }
std::string Id() { return "combat_"+std::to_string(++occurrence); }
std::string Bytes(const campaign::CampaignCore& c) {
  std::ostringstream out; Check(c.Save(out)==save::Result::Ok,"Complete combat checkpoint serializes coherently"); return out.str();
}
save::Result Load(campaign::CampaignCore& c, const std::string& bytes) { std::istringstream in(bytes); return c.Load(in); }
campaign::CombatContext Context(const std::string& node="F4_START") { return {node,true,true,true}; }
campaign::CombatCommand Command(const campaign::CampaignCore& c, CombatOp kind, const std::string& target="rifle", std::uint64_t serial=0) {
  campaign::CombatCommand command;
  command.eventId=Id(); command.kind=kind; command.node="F4_START"; command.target=target; command.serial=serial; command.atSeconds=c.Now();
  if(kind==CombatOp::Fire) command.vector={1,0,0};
  return command;
}
campaign::CombatResult Apply(campaign::CampaignCore& c, const campaign::CombatCommand& command, const campaign::CombatContext& ctx=Context()) {
  auto result=c.ApplyCombat(command,ctx); Check(result.status==Status::Applied,"Physical combat command confirms"); Bytes(c); return result;
}
void Rejected(campaign::CampaignCore& c, const campaign::CombatCommand& command, Status status, const campaign::CombatContext& ctx=Context()) {
  const auto before=Bytes(c); Check(c.ApplyCombat(command,ctx).status==status,"Combat denial reports expected status");
  Check(Bytes(c)==before,"Rejected combat preserves every system and receipt");
}
int Stock(const campaign::CampaignCore& c,const std::string& item) {
  const auto inventory=c.Inventory(); const auto it=inventory.contents.find(item); return it==inventory.contents.end()?0:it->second;
}
campaign::CombatTask Reload(campaign::CampaignCore& c) {
  const auto task=*Apply(c,Command(c,CombatOp::BeginReload)).task;
  Check(c.Tick(task.readyAt-c.Now(),survival::Activity::Rest).status==Status::Applied,"Reload simulation advances without implicit animation confirmation");
  Apply(c,Command(c,CombatOp::CompleteReload,"rifle",task.serial)); return task;
}
campaign::ThrownItem Throw(campaign::CampaignCore& c,const std::string& item) {
  const auto task=*Apply(c,Command(c,CombatOp::BeginThrow,item)).task;
  c.Tick(task.readyAt-c.Now(),survival::Activity::Rest);
  auto command=Command(c,CombatOp::ReleaseThrow,item,task.serial); command.position={2,0,0}; command.vector={3,0,2};
  return *Apply(c,command).thrown;
}
void ReachFuse(campaign::CampaignCore& c,const campaign::ThrownItem& item) {
  auto delta=c.Tick(100,survival::Activity::Rest);
  Check(delta.status==Status::Applied&&c.Now()==item.detonateAt,"Long delta stops exactly at earliest physical fuse");
  const auto before=Bytes(c);
  Check(c.Tick(1,survival::Activity::Rest).status==Status::AwaitingPhysics&&Bytes(c)==before,"Unresolved explosion cannot silently advance time or grant effects");
}
campaign::CombatCommand Detonate(const campaign::CampaignCore& c,const campaign::ThrownItem& item) {
  auto command=Command(c,CombatOp::Detonate,item.item,item.serial);
  command.node=item.node; command.position=item.position; return command;
}
void RoundTrip(campaign::CampaignCore& c,rules::Difficulty mode) {
  const auto bytes=Bytes(c); campaign::CampaignCore restored(mode,98765);
  Check(Load(restored,bytes)==save::Result::Ok&&Bytes(restored)==bytes,"Combat state resumes byte-for-byte with different constructor seed");
}

void WeaponAndReload() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    campaign::CampaignCore c(mode,9); const auto p=campaign::CombatProfile(mode); const int stock=Stock(c,"magazine");
    Check(c.CombatRuntime().loadedRounds==0&&Stock(c,"rifle")==1,"Authored magazines are conserved before the first reload");
    Rejected(c,Command(c,CombatOp::Fire),Status::MissingRequirement);
    auto task=*Apply(c,Command(c,CombatOp::BeginReload)).task;
    Check(Stock(c,"magazine")==stock,"Begin reload reserves stock without consuming it");
    RoundTrip(c,mode);
    Rejected(c,Command(c,CombatOp::BeginReload),Status::Conflict);
    Rejected(c,Command(c,CombatOp::CompleteReload,"rifle",task.serial),Status::MissingRequirement);
    Rejected(c,Command(c,CombatOp::Fire),Status::Conflict);
    c.SetPaused(true); const auto paused=Bytes(c);
    Check(c.Tick(20,survival::Activity::Rest).status==Status::Paused&&Bytes(c)==paused,"Pause freezes reload timer");
    Rejected(c,Command(c,CombatOp::CancelReload,"rifle",task.serial),Status::Paused);
    c.SetPaused(false);
    Apply(c,Command(c,CombatOp::CancelReload,"rifle",task.serial));
    Check(Stock(c,"magazine")==stock&&c.CombatRuntime().loadedRounds==0,"Cancellation releases reservation without losing ammo");
    task=Reload(c);
    Check(Stock(c,"magazine")==stock-1&&c.CombatRuntime().loadedRounds==p.magazineRounds,"Completed animation installs exactly one magazine");
    Rejected(c,Command(c,CombatOp::BeginReload),Status::MissingRequirement);
    auto fire=Command(c,CombatOp::Fire); fire.position={1,2,3};
    auto shot=*Apply(c,fire).shot;
    Check(c.CombatRuntime().loadedRounds==p.magazineRounds-1&&c.Hunter().state==hunter::State::Patrol,"Shot spends one round without telepathic detection");
    const auto fired=Bytes(c);
    Check(c.ApplyCombat(fire,Context()).status==Status::AlreadyApplied&&Bytes(c)==fired,"Replay never respawns projectile or spends second round");
    auto altered=fire; altered.position.x=4; Rejected(c,altered,Status::Conflict);
    Rejected(c,Command(c,CombatOp::Fire),Status::MissingRequirement);
    auto miss=Command(c,CombatOp::ResolveShot,"miss",shot.serial);
    auto noCollision=Context();noCollision.collisionVerified=false;
    Rejected(c,miss,Status::MissingRequirement,noCollision); Apply(c,miss);
    Rejected(c,Command(c,CombatOp::ResolveShot,"miss",shot.serial),Status::MissingRequirement);
    RoundTrip(c,mode);
    if(stock>1) {
      Reload(c);
      Check(Stock(c,"magazine")==stock-2&&c.CombatRuntime().loadedRounds==p.magazineRounds,"Tactical reload discards residual rounds without returning a full magazine");
    }
    // Empty a loaded magazine through distinct physical shots, resolving misses.
    while(c.CombatRuntime().loadedRounds>0) {
      c.Tick(.2,survival::Activity::Rest);
      auto current=*Apply(c,Command(c,CombatOp::Fire)).shot;
      Apply(c,Command(c,CombatOp::ResolveShot,"miss",current.serial));
    }
    Rejected(c,Command(c,CombatOp::Fire),Status::MissingRequirement);
    RoundTrip(c,mode);
  }
}
void ReservationsAndPhysicalProof() {
  campaign::CampaignCore c(rules::Difficulty::Punitive);
  const auto task=*Apply(c,Command(c,CombatOp::BeginReload)).task;
  campaign::Action share{Id(),campaign::ActionKind::ShareAmmo,"harley","F4_START",c.Now(),0,true};
  auto before=Bytes(c);Check(c.Apply(share).status==Status::MissingRequirement&&Bytes(c)==before,"Sharing cannot take the last reserved magazine");
  campaign::CompanionCommand transfer;
  transfer.eventId=Id();transfer.kind=campaign::CompanionOp::Transfer;transfer.actor="joe";transfer.target="harley";
  transfer.node="F4_START";transfer.item="magazine";transfer.quantity=1;transfer.atSeconds=c.Now();transfer.physicallyVerified=true;
  campaign::CompanionContext ctx;ctx.actionCompleted=true;ctx.actors["joe"]={"F4_START",true};ctx.actors["harley"]={"F4_START",true};
  before=Bytes(c);Check(c.ApplyCompanion(transfer,ctx).status==Status::MissingRequirement&&Bytes(c)==before,"Companion transfer respects weapon reservation");
  c.Tick(task.readyAt-c.Now(),survival::Activity::Rest);
  auto finish=Command(c,CombatOp::CompleteReload,"rifle",task.serial);
  auto noAnimation=Context();noAnimation.animationCompleted=false;
  Rejected(c,finish,Status::MissingRequirement,noAnimation);
  auto noProof=Context();noProof.physicallyVerified=false;Rejected(c,finish,Status::InvalidAction,noProof);
  auto wrongNode=Context("F3_EXAMS");Rejected(c,finish,Status::MissingRequirement,wrongNode);
  Rejected(c,Command(c,CombatOp::CompleteReload,"rifle",task.serial+1),Status::MissingRequirement);
  Apply(c,finish);
  RoundTrip(c,rules::Difficulty::Punitive);
  auto fire=Command(c,CombatOp::Fire);fire.vector={0,0,0};Rejected(c,fire,Status::InvalidAction);
  fire=Command(c,CombatOp::Fire);fire.position.x=std::numeric_limits<double>::quiet_NaN();Rejected(c,fire,Status::InvalidAction);
  fire=Command(c,CombatOp::Fire);fire.atSeconds+=1;Rejected(c,fire,Status::InvalidTime);
  auto shot=*Apply(c,Command(c,CombatOp::Fire)).shot;
  c.Tick(2,survival::Activity::Rest);
  Check(c.CombatRuntime().shots.empty(),"Unconfirmed shots expire without automatic hits");
  Rejected(c,Command(c,CombatOp::ResolveShot,"hunter",shot.serial),Status::MissingRequirement);
}
void HunterInterrupts() {
  campaign::CampaignCore c(rules::Difficulty::Hard);Reload(c);
  const auto fire=Command(c,CombatOp::Fire);const auto result=Apply(c,fire);
  Check(result.sound&&result.sound->kind==perception::SoundKind::Gunshot,"Confirmed shot supplies one perception event at muzzle");
  perception::ListenerContext listener{{0,0,0},1,0};
  Check(c.Hear(*result.sound,listener)&&c.Hunter().lastHeardType=="gunshot","Shot travels through actual hearing pipeline");
  auto impact=Command(c,CombatOp::ResolveShot,"hunter",result.shot->serial);
  Check(Apply(c,impact).hunterInterrupted,"Verified Hunter impact opens temporary interruption");
  campaign::HunterContext hc;hc.actualHunterNode="F4_START";hc.physicallyVerified=true;hc.reachableNodes={"F4_START"};
  Check(c.HunterOffers(hc).empty(),"Hunter cannot begin tactics during interruption");
  campaign::HunterCommand choose{Id(),campaign::HunterOp::Choose,"","","F4_START",c.Now()};
  const auto before=Bytes(c);Check(c.ApplyHunter(choose,hc).status==Status::MissingRequirement&&Bytes(c)==before,"Direct callback cannot bypass interruption");
  c.Tick(.2,survival::Activity::Rest);
  auto next=*Apply(c,Command(c,CombatOp::Fire)).shot;
  Check(!Apply(c,Command(c,CombatOp::ResolveShot,"hunter",next.serial)).hunterInterrupted,"Repeated shots cannot extend stun into a combat lock");
  RoundTrip(c,rules::Difficulty::Hard);
  c.Tick(1,survival::Activity::Rest);Check(!c.HunterOffers(hc).empty(),"Hunter resumes tactics without losing remembered evidence");
}
void GrenadesSmokeAndCheckpoint() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    campaign::CampaignCore c(mode);const int stock=Stock(c,"smoke");
    const auto prep=*Apply(c,Command(c,CombatOp::BeginThrow,"smoke")).task;
    Rejected(c,Command(c,CombatOp::ReleaseThrow,"smoke",prep.serial),Status::MissingRequirement);
    Apply(c,Command(c,CombatOp::CancelThrow,"smoke",prep.serial));Check(Stock(c,"smoke")==stock,"Cancelled preparation has no live grenade and no cost");
    auto smoke=Throw(c,"smoke");Check(Stock(c,"smoke")==stock-1,"Release consumes exactly one canister");RoundTrip(c,mode);
    Check(c.TrackThrown(smoke.serial,"F3_EXAMS",{3,0,0},{0,1,0},true),"Physical projectile tracking updates persisted pose and velocity");
    smoke=c.CombatRuntime().thrown.at(smoke.serial);RoundTrip(c,mode);
    Check(!c.TrackThrown(smoke.serial+99,"F3_EXAMS",{},{},true),"Stale physical projectile ID cannot create a grenade");
    Rejected(c,Detonate(c,smoke),Status::MissingRequirement);
    if(mode==rules::Difficulty::Normal) {
      campaign::HunterCommand sight{Id(),campaign::HunterOp::SeeActor,"joe","",smoke.node,c.Now()};sight.observedPosition={3,0,0};
      campaign::HunterContext hc;hc.actualHunterNode="F4_START";hc.physicallyVerified=hc.lineOfSightVerified=true;
      Check(c.SenseHunter(sight,hc),"Hunter perceives Joe before cloud becomes active");
    }
    ReachFuse(c,smoke);const auto detonate=Detonate(c,smoke);
    Apply(c,detonate);const auto after=Bytes(c);
    Check(c.ApplyCombat(detonate,Context()).status==Status::AlreadyApplied&&Bytes(c)==after,"Detonation replay creates no duplicate smoke field");
    Check(c.CombatRuntime().thrown.empty()&&c.CombatRuntime().smoke.size()==1,"Detonation exchanges projectile for one temporary smoke field");
    Check(c.SmokeOccludes({-10,0,0},{10,0,0})&&c.SmokeOccludes({3,0,0},{3,0,0}),"Segment crossing and observer inside smoke are occluded");
    Check(!c.SmokeOccludes({-10,10,0},{10,10,0}),"Smoke in another corridor does not globally blind Hunter");
    Check(c.Hunter().state==(mode==rules::Difficulty::Normal?hunter::State::Pursuing:hunter::State::Patrol),"Smoke alone does not invent a sighting or erase a valid pursuit");
    if(mode==rules::Difficulty::Normal) {
      campaign::HunterCommand loss{Id(),campaign::HunterOp::LoseActor,"joe","","F4_START",c.Now()};
      campaign::HunterContext hc;hc.actualHunterNode="F4_START";hc.physicallyVerified=true;
      Check(c.SenseHunter(loss,hc)&&c.Hunter().state==hunter::State::Searching&&c.HunterRuntime().actors.at("joe").node==smoke.node,"Real sensor loss in smoke retains last seen location instead of following hidden Joe");
    }
    RoundTrip(c,mode);
    c.SetPaused(true);const auto paused=Bytes(c);Check(c.Tick(100,survival::Activity::Rest).status==Status::Paused&&Bytes(c)==paused,"Pause freezes smoke lifetime");c.SetPaused(false);
    c.Tick(campaign::CombatProfile(mode).smokeSeconds,survival::Activity::Rest);
    Check(c.CombatRuntime().smoke.empty()&&!c.SmokeOccludes({-10,0,0},{10,0,0}),"Expired smoke restores visibility query");
  }
  campaign::CampaignCore c(rules::Difficulty::Hard);auto grenade=Throw(c,"grenade");ReachFuse(c,grenade);
  auto command=Detonate(c,grenade);command.contacts={{"harley","F4_START",{2,0,0},survival::Injury::Moderate},{"hunter","F4_START",{2,0,0},survival::Injury::None},{"joe","F4_START",{2,0,0},survival::Injury::Severe}};
  auto invalid=command;invalid.contacts.back().position={200,0,0};Rejected(c,invalid,Status::MissingRequirement);
  Check(Apply(c,command).hunterInterrupted,"Verified blast temporarily interrupts Hunter");
  Check(c.Survival().injury==survival::Injury::Severe&&c.CompanionRuntime(companion::Character::Harley).survival.injury==survival::Injury::Moderate,"Complete physical blast query applies individual injuries together");
  Check(c.CombatRuntime().thrown.empty()&&Stock(c,"grenade")==0,"Detonation removes thrown object without second consumption");RoundTrip(c,rules::Difficulty::Hard);
}
void Destructibles() {
  auto d=campaign::data::Hospital();d.objects.push_back({"TEST_PANEL","destructible",{"intact",{},{},true}});
  d.destructibles.push_back({"TEST_PANEL","F4_START","intact","broken",true,true});
  campaign::CampaignCore c(rules::Difficulty::Hard,d);Reload(c);
  const auto shot=*Apply(c,Command(c,CombatOp::Fire)).shot;
  Apply(c,Command(c,CombatOp::ResolveShot,"TEST_PANEL",shot.serial));
  Check(c.World().objects.at("TEST_PANEL").stateId=="broken","Authored physical impact changes only explicit destructible state");
  const auto bytes=Bytes(c);campaign::CampaignCore loaded(rules::Difficulty::Hard,d);Check(Load(loaded,bytes)==save::Result::Ok,"Destructible state resumes with combat proof");
  c.Tick(.2,survival::Activity::Rest);const auto another=*Apply(c,Command(c,CombatOp::Fire)).shot;
  Apply(c,Command(c,CombatOp::ResolveShot,"TEST_PANEL",another.serial));
  Check(c.World().objects.at("TEST_PANEL").stateId=="broken","Concurrent impact on already broken object resolves without blocking combat");
  c.Tick(.2,survival::Activity::Rest);const auto blocked=*Apply(c,Command(c,CombatOp::Fire)).shot;
  Rejected(c,Command(c,CombatOp::ResolveShot,"SAFE_F2",blocked.serial),Status::InvalidAction);
  Apply(c,Command(c,CombatOp::ResolveShot,"miss",blocked.serial));
  d.destructibles[0].after="debris";campaign::CampaignCore incompatible(rules::Difficulty::Hard,d);
  Check(Load(incompatible,bytes)==save::Result::IncompatibleRoster,"Destructible authoring participates in content identity");
}
void BreathingAndTiming() {
  for(auto mode:{rules::Difficulty::Easy,rules::Difficulty::Normal,rules::Difficulty::Hard,rules::Difficulty::Punitive}) {
    campaign::CampaignCore c(mode);const auto p=campaign::CombatProfile(mode);
    Check(c.BreathingSound({}).has_value(),"Calm breathing retains perception energy");
    Apply(c,Command(c,CombatOp::HoldBreath,"joe"));Check(!c.BreathingSound({}),"Holding suppresses emission, not Hunter's existing memory");
    c.Tick(2,survival::Activity::Sneak);Check(c.Breathing().oxygen<p.oxygenCapacity&&c.Breathing().holding,"Quiet movement consumes finite oxygen");RoundTrip(c,mode);
    c.SetPaused(true);const auto paused=Bytes(c);Check(c.Tick(100,survival::Activity::Rest).status==Status::Paused&&Bytes(c)==paused,"Paused hold cannot lose oxygen");c.SetPaused(false);
    auto delta=c.Tick(100,survival::Activity::Rest);
    Check(delta.forcedBreathRelease&&!c.Breathing().holding&&c.Breathing().gasping&&c.Breathing().oxygen==0,"Long delta stops at oxygen depletion and exposes the audible gasp");
    Check(c.Ending()==rules::Ending::None&&c.BreathingSound({})->energy>1.0/3,"Depletion produces noise rather than oxygen death");
    Rejected(c,Command(c,CombatOp::HoldBreath,"joe"),Status::MissingRequirement);
    c.Tick(1,survival::Activity::Rest);Rejected(c,Command(c,CombatOp::HoldBreath,"joe"),Status::MissingRequirement);
    c.Tick(6,survival::Activity::Rest);Apply(c,Command(c,CombatOp::HoldBreath,"joe"));
    auto sprint=c.Tick(10,survival::Activity::Sprint);
    Check(sprint.advancedSeconds==0&&sprint.forcedBreathRelease&&!c.Breathing().holding,"Attempted sprint releases breath before movement time advances");
    RoundTrip(c,mode);
  }
  campaign::CampaignCore c(rules::Difficulty::Hard);
  campaign::Action injury{Id(),campaign::ActionKind::Injury,"joe","F4_START",c.Now(),3,true};Check(c.Apply(injury).status==Status::Applied,"Injury fixture confirms");
  Check(c.Breathing().energy>1.0/3,"Injured breathing remains louder without holding");
  Apply(c,Command(c,CombatOp::HoldBreath,"joe"));const auto release=c.Tick(100,survival::Activity::Rest);
  Check(release.advancedSeconds<100.0/7&&release.forcedBreathRelease,"Injury shortens holding reserve");RoundTrip(c,rules::Difficulty::Hard);
}
void CorruptionAndEnding() {
  campaign::CampaignCore c(rules::Difficulty::Hard);Reload(c);Apply(c,Command(c,CombatOp::Fire));
  const auto good=Bytes(c);
  auto corrupt=good;const auto start=corrupt.find("COMBAT ");corrupt.replace(start+7,2,"31");
  const auto checksum=corrupt.rfind("CHECKSUM ");corrupt=corrupt.substr(0,checksum)+"CHECKSUM "+campaign::detail::Hash(corrupt.substr(0,checksum))+"\n";
  Check(Load(c,corrupt)==save::Result::IncompatibleRoster&&Bytes(c)==good,"Semantically invalid ammo is rejected even with valid checksum");
  auto legacy=good;legacy.replace(0,std::string("THE_THUG_RUNTIME_CAMPAIGN 6").size(),"THE_THUG_RUNTIME_CAMPAIGN 4");
  Check(Load(c,legacy)==save::Result::UnsupportedVersion&&Bytes(c)==good,"Legacy checkpoint never silently loses combat state");
  campaign::Action capture{Id(),campaign::ActionKind::Trigger,"captured","F4_START",c.Now(),0,true};
  Check(c.Apply(capture).status==Status::Applied&&c.CombatRuntime().shots.empty(),"Ending clears pending presentation/world combat work");
  Rejected(c,Command(c,CombatOp::Fire),Status::Finished);RoundTrip(c,rules::Difficulty::Hard);
}
void InterruptionsAndSimultaneousFuses() {
  campaign::CampaignCore c(rules::Difficulty::Hard);
  const int initial=Stock(c,"magazine");
  const auto task=*Apply(c,Command(c,CombatOp::BeginReload)).task;
  campaign::Action injury{Id(),campaign::ActionKind::Injury,"joe","F4_START",c.Now(),1,true};
  Check(c.Apply(injury).status==Status::Applied&&!c.CombatRuntime().task&&Stock(c,"magazine")==initial,"Injury interrupts reload without consuming reserved stock");
  Rejected(c,Command(c,CombatOp::CompleteReload,"rifle",task.serial),Status::MissingRequirement);
  RoundTrip(c,rules::Difficulty::Hard);
  Reload(c);
  const auto hide=campaign::data::Hospital().hides.front();
  const auto pendingShot=*Apply(c,Command(c,CombatOp::Fire)).shot;
  campaign::Action enter{Id(),campaign::ActionKind::EnterHide,hide.id,hide.atNode,c.Now(),0,true};
  Check(c.Apply(enter).status==Status::Applied,"Physical hide fixture occupied");
  const auto wrong=hide.atNode=="F4_START"?"F3_EXAMS":"F4_START";
  Rejected(c,Command(c,CombatOp::ResolveShot,"miss",pendingShot.serial),Status::MissingRequirement,Context(wrong));
  Apply(c,Command(c,CombatOp::ResolveShot,"miss",pendingShot.serial),Context(hide.atNode));
  auto hidden=Command(c,CombatOp::Fire);hidden.node=hide.atNode;
  Rejected(c,hidden,Status::MissingRequirement,Context(hide.atNode));
  auto hold=Command(c,CombatOp::HoldBreath,"joe");hold.node=hide.atNode;Apply(c,hold,Context(hide.atNode));
  RoundTrip(c,rules::Difficulty::Hard);
  campaign::CampaignCore fuses(rules::Difficulty::Hard);
  const auto grenade=Throw(fuses,"grenade");
  fuses.Tick(1.5,survival::Activity::Rest);
  const auto smoke=Throw(fuses,"smoke");
  Check(smoke.detonateAt==grenade.detonateAt,"Distinct physical devices can share one fuse boundary");
  ReachFuse(fuses,grenade);RoundTrip(fuses,rules::Difficulty::Hard);
  auto noCollision=Context();noCollision.collisionVerified=false;
  Rejected(fuses,Detonate(fuses,grenade),Status::MissingRequirement,noCollision);
  Apply(fuses,Detonate(fuses,grenade));
  Check(fuses.Tick(1,survival::Activity::Rest).status==Status::AwaitingPhysics,"Second due device prevents advancing after first detonation");
  Apply(fuses,Detonate(fuses,smoke));Check(fuses.Tick(1,survival::Activity::Rest).status==Status::Applied,"Resolving every due device releases simulation clock");
  RoundTrip(fuses,rules::Difficulty::Hard);
  campaign::CampaignCore fatal(rules::Difficulty::Hard);
  const auto explosive=Throw(fatal,"grenade");ReachFuse(fatal,explosive);
  auto blast=Detonate(fatal,explosive);
  blast.contacts={{"harley","F4_START",explosive.position,survival::Injury::Severe}};
  Apply(fatal,blast);fatal.Tick(300,survival::Activity::Rest);
  Check(fatal.Narrative().outcomes.at("harley").dead&&fatal.Ending()==rules::Ending::None,"Blast-injured companion can die individually while Joe survives");
  RoundTrip(fatal,rules::Difficulty::Hard);
  survival::Tuning tuning;tuning.bleedingPerSecond=100;
  campaign::CampaignCore death(rules::Difficulty::Hard,campaign::data::Hospital(),1,tuning);
  campaign::Action bleeding{Id(),campaign::ActionKind::Injury,"joe","F4_START",death.Now(),3,true};
  Check(death.Apply(bleeding).status==Status::Applied,"Fatal bleeding fixture confirms");
  Throw(death,"grenade");const auto terminal=death.Tick(100,survival::Activity::Rest);
  Check(terminal.advancedSeconds==.5&&death.CombatRuntime().thrown.empty()&&death.Ending()==rules::Ending::Died,"Earlier fatal boundary clears pending grenade without advancing past death");
  Bytes(death);
}
void CompletionReceiptBudget() {
  // File fixture represents 9,996 distinct verified hold/release pairs at one
  // clock instant. Avoid quadratic setup through thousands of state copies.
  campaign::CampaignCore c(rules::Difficulty::Hard);
  auto bytes=Bytes(c);
  const auto combat=bytes.find("COMBAT ");const auto lineEnd=bytes.find('\n',combat);
  bytes.replace(combat,lineEnd-combat,"COMBAT 0 0 9996 -1 0 0 100 0 0");
  std::ostringstream ledger;ledger<<"RECEIPTS 9996\n";
  for(std::uint64_t i=1;i<=9996;++i) {
    auto command=Command(c,i%2?CombatOp::HoldBreath:CombatOp::ReleaseBreath,"joe");
    ledger<<"RECEIPT "<<std::quoted(command.eventId)<<' '<<std::quoted(campaign::detail::Receipt(command,i,0))<<'\n';
  }
  const auto receiptStart=bytes.find("RECEIPTS 0\n");bytes.replace(receiptStart,std::string("RECEIPTS 0\n").size(),ledger.str());
  const auto checksum=bytes.rfind("CHECKSUM ");bytes=bytes.substr(0,checksum)+"CHECKSUM "+campaign::detail::Hash(bytes.substr(0,checksum))+"\n";
  Check(Load(c,bytes)==save::Result::Ok,"Near-budget checkpoint fixture validates as a coherent session");
  const auto task=*Apply(c,Command(c,CombatOp::BeginThrow,"grenade")).task;
  Apply(c,Command(c,CombatOp::HoldBreath,"joe"));
  Rejected(c,Command(c,CombatOp::ReleaseBreath,"joe"),Status::LogFull);
  c.Tick(task.readyAt-c.Now(),survival::Activity::Rest);
  auto release=Command(c,CombatOp::ReleaseThrow,"grenade",task.serial);release.position={2,0,0};
  const auto grenade=*Apply(c,release).thrown;
  Rejected(c,Command(c,CombatOp::HoldBreath,"joe"),Status::LogFull);
  ReachFuse(c,grenade);Apply(c,Detonate(c,grenade));
  Check(c.AppliedActionCount()==10000&&c.CombatRuntime().thrown.empty()&&c.Tick(1,survival::Activity::Rest).status==Status::Applied,"Reserved receipts resolve live grenade even at the session ledger limit");
  Bytes(c);
}
void HiddenCompanionBlastProof() {
  campaign::CampaignCore c(rules::Difficulty::Hard);
  const auto hide=campaign::data::Hospital().hides.front();
  campaign::CompanionContext context;
  context.actors["harley"]={hide.atNode,true};context.reachableNodes={hide.atNode};
  context.actionCompleted=context.movementCompleted=true;
  campaign::CompanionCommand command;
  command.eventId=Id();command.kind=campaign::CompanionOp::ObserveHide;command.actor="harley";
  command.target=hide.id;command.node=hide.atNode;command.atSeconds=c.Now();command.physicallyVerified=true;
  Check(c.ApplyCompanion(command,context).status==Status::Applied,"Harley sees physical hide");
  command.eventId=Id();command.kind=campaign::CompanionOp::BeginTask;
  const auto begin=c.ApplyCompanion(command,context);Check(begin.status==Status::Applied&&begin.task.has_value(),"Harley prepares observed hide");
  command.eventId=Id();command.kind=campaign::CompanionOp::Arrive;command.taskSerial=begin.task->serial;
  Check(c.ApplyCompanion(command,context).status==Status::Applied,"Harley reaches hide");
  c.Tick(campaign::partydata::InteractionSeconds[0],survival::Activity::Rest);
  command.eventId=Id();command.kind=campaign::CompanionOp::Complete;command.atSeconds=c.Now();
  Check(c.ApplyCompanion(command,context).status==Status::Applied,"Harley occupies hide");
  auto grenade=Throw(c,"grenade");ReachFuse(c,grenade);
  auto blast=Detonate(c,grenade);
  blast.contacts={{"harley",hide.atNode=="F4_START"?"F3_EXAMS":"F4_START",grenade.position,survival::Injury::Moderate}};
  Rejected(c,blast,Status::MissingRequirement);
  blast.contacts[0].node=hide.atNode;Apply(c,blast);
  Check(c.CompanionRuntime(companion::Character::Harley).hidingPlace==hide.id,"Valid hit preserves the hidden companion's physical location");
  RoundTrip(c,rules::Difficulty::Hard);
}
}
int main() {
  try { WeaponAndReload();ReservationsAndPhysicalProof();HunterInterrupts();GrenadesSmokeAndCheckpoint();Destructibles();BreathingAndTiming();CorruptionAndEnding();InterruptionsAndSimultaneousFuses();CompletionReceiptBudget();HiddenCompanionBlastProof(); }
  catch(const std::exception& error) { std::cerr<<"Combat check "<<checks<<" failed: "<<error.what()<<'\n';return 1; }
  std::cout<<"Campaign combat: "<<checks<<" checks passed\n";
}
