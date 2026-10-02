#include "CampaignState.h"
#include "HunterIO.h"
#include <algorithm>
namespace thug::campaign {
namespace {
template<class T>const T* Find(const std::vector<T>& list,const std::string& id){
  for(const auto& v:list)if(v.id==id)return &v;
  return nullptr;
}
bool Blocking(HunterGoal g){
  return g==HunterGoal::CloseDoor||g==HunterGoal::PowerOff;
}
bool Environmental(HunterGoal g){
  return g>=HunterGoal::CloseDoor&&g<=HunterGoal::DisableCamera;
}
bool Live(const HunterTrack& t,double now,const HunterTuning& p){
  return t.visible&&now-t.estimate.observedAtSeconds<=p.visualLease;
}
}
HunterTuning HunterProfile(rules::Difficulty mode){
  switch(mode){
    case rules::Difficulty::Easy:return {2,.03,5,180,20,70,.1};
    case rules::Difficulty::Normal:return {2,.02,4,140,30,50,.25};
    case rules::Difficulty::Hard:return {2,.012,3,100,40,35,.45};
    case rules::Difficulty::Punitive:return {2,.008,2,70,50,20,.7};
  }
  throw std::invalid_argument("Unknown Hunter policy difficulty");
}
HunterRuntimeSnapshot CampaignCore::HunterRuntime()const{
  auto value=state_->hunterRuntime;
  std::ostringstream out;
  out<<state_->hunterRandom;
  value.randomState=out.str();
  return value;
}
void CampaignCore::AdvanceHunter(State& s,double now)const{
  auto& r=s.hunterRuntime;
  const auto p=HunterProfile(s.routes.DifficultyLevel());
  r.aggression=std::max(0.0,r.aggression-(now-r.nowSeconds)*p.aggressionDecay);
  r.nowSeconds=now;
  for(auto& [actor,t]:r.actors)if(t.visible&&!Live(t,now,p)){
    t.visible=false;
    if(actor=="joe"&&s.hunter.CurrentState()==hunter::State::Pursuing)s.hunter.Observe({hunter::StimulusKind::VisualLost,hunter::Subject::Player,std::max(s.hunter.CaptureSnapshot().nowSeconds,std::nextafter(t.estimate.observedAtSeconds+p.visualLease,now)),{} ,1,""});
    if(r.task&&r.task->goal==HunterGoal::Pursue&&r.task->target==actor)r.task.reset();
  }
  if(r.lastAmbushAt>=0&&now-r.lastAmbushAt>=p.watchSeconds)r.watchingDoor.clear();
}
bool CampaignCore::HunterStaticValid(const HunterCommand& c)const{
  if(!detail::Token(c.eventId)||!definition_->nodes.count(c.node)||!std::isfinite(c.atSeconds)||c.atSeconds<0||c.kind<HunterOp::SeeActor||c.kind>HunterOp::Cancel||!detail::Finite(c.observedPosition)||!std::isfinite(c.radiusMeters)||c.radiusMeters<0)return false;
  if(c.kind!=HunterOp::SeeActor&&(c.radiusMeters||c.observedPosition.x||c.observedPosition.y||c.observedPosition.z))return false;
  if(c.kind==HunterOp::SeeActor)return detail::Actor(c.actor)&&c.target.empty()&&!c.taskSerial&&c.argument>=0&&c.argument<=1;
  if(c.kind==HunterOp::LoseActor)return detail::Actor(c.actor)&&c.target.empty()&&!c.taskSerial&&!c.argument;
  if(c.kind==HunterOp::Pressure)return (c.actor.empty()||detail::Actor(c.actor))&&c.target.empty()&&!c.taskSerial&&c.argument>=0&&c.argument<=3&&(c.argument!=3||!c.actor.empty());
  if(c.kind==HunterOp::NoticePassage)return detail::Actor(c.actor)&&Find(definition_->doors,c.target)&&!c.taskSerial&&!c.argument;
  if(!c.actor.empty()||c.argument)return false;
  if(c.kind==HunterOp::Choose)return c.target.empty()&&!c.taskSerial;
  return detail::Token(c.target)&&c.taskSerial>0;
}
std::vector<HunterOffer> CampaignCore::HunterOffers(const HunterContext& ctx)const{
  if(Paused()||Ending()!=rules::Ending::None||state_->hunterRuntime.task||Now()<state_->combat.hunterBlockedUntil||state_->special.phase==SpecialPhase::Collapsing)return {};
  return HunterOffers(*state_,ctx);
}
std::vector<HunterOffer> CampaignCore::HunterOffers(const State& s,const HunterContext& ctx)const{
  std::vector<HunterOffer> offers;
  if(s.world.NowSeconds()<s.combat.hunterBlockedUntil)return offers;
  if(s.special.phase==SpecialPhase::Opportunity && s.world.NowSeconds()<s.special.boundaryAt)return offers;
  if(!definition_->nodes.count(ctx.actualHunterNode))return offers;
  for(const auto& n:ctx.reachableNodes)if(!definition_->nodes.count(n))return {};
  const auto& r=s.hunterRuntime;
  const double now=r.nowSeconds;
  const auto p=HunterProfile(s.routes.DifficultyLevel());
  const auto add=[&](HunterGoal g,const std::string& target,const std::string& node,double w){
    if(ctx.reachableNodes.count(node))offers.push_back({g,target,node,w});
  };
  const auto story=s.narrative.CaptureSnapshot();
  for(const auto& [actor,t]:r.actors){
    if(actor!="joe"){
      const auto& o=story.outcomes.at(actor);
      if(o.dead||o.captured||o.escapedAlone||o.together)continue;
    }
    if(Live(t,now,p))add(HunterGoal::Pursue,actor,t.node,(actor=="joe"?3.0:2.0)+(t.threatening?6:0));
  }
  if(!offers.empty())return offers;
  const auto intent=s.hunter.CurrentIntent();
  if(intent.destination)for(const auto& [node,center]:ctx.searchSectors){
    const auto visited=r.searched.find(node);
    if(detail::Finite(center)&&definition_->nodes.count(node)&&detail::Distance(center,intent.destination->center)<=intent.destination->radiusMeters&&(visited==r.searched.end()||now-visited->second>=30))add(HunterGoal::Investigate,node,node,5);
  }
  for(const auto& [actor,t]:r.actors)if(!Live(t,now,p)&&now-t.estimate.observedAtSeconds<=s.hunter.CaptureSnapshot().tuning.locationLifetimeSeconds*(1+r.aggression/100)){
    (void)actor;
    const auto visited=r.searched.find(t.node);
    if(visited==r.searched.end()||now-visited->second>=30)add(HunterGoal::Investigate,t.node,t.node,3);
  }
  for(const auto& hide:definition_->hides)if(s.hunter.HideSpotSuspicion(hide.id)>0){
    const auto visited=r.searched.find(hide.id);
    if(visited==r.searched.end()||now-visited->second>=30)add(HunterGoal::InspectHide,hide.id,hide.atNode,1+s.hunter.HideSpotSuspicion(hide.id)/30);
  }
  const auto seen=s.evidence.CaptureSnapshot().lastSeen;
  for(const auto& [id,time]:r.passages)if(now-time<=s.hunter.CaptureSnapshot().tuning.locationLifetimeSeconds*(1+r.aggression/100)&&(r.lastAmbushAt<0||now-r.lastAmbushAt>=p.ambushCooldown)){
    const auto* d=Find(definition_->doors,id);
    if(d){
      add(HunterGoal::WatchPassage,id,d->fromNode,p.ambushWeight*(1+r.aggression/50));
      add(HunterGoal::WatchPassage,id,d->toNode,p.ambushWeight*(1+r.aggression/50));
    }
  }
  if(r.aggression>=p.environmentThreshold){
    for(const auto& d:definition_->doors){
      const auto memory=seen.find(d.id);
      if(memory==seen.end())continue;
      const bool open=memory->second=="open"||memory->second=="forced_open";
      if(open&&!ctx.safeToAlter.count(d.id))continue;
      if(!open&&memory->second!="closed")continue;
      const auto g=open?HunterGoal::CloseDoor:HunterGoal::OpenDoor;
      add(g,d.id,d.fromNode,1+r.aggression/100);
      add(g,d.id,d.toNode,1+r.aggression/100);
    }
    for(const auto& [id,node]:definition_->circuitNodes){
      const auto memory=seen.find(id);
      if(memory!=seen.end()&&memory->second=="on"&&ctx.safeToAlter.count(id))add(HunterGoal::PowerOff,id,node,1+r.aggression/100);
    }
    for(const auto& cam:definition_->cameras){
      const auto memory=seen.find(cam.id);
      if(memory!=seen.end()&&memory->second=="online")add(HunterGoal::DisableCamera,cam.id,cam.atNode,1+r.aggression/100);
    }
  }
  for(const auto& node:ctx.reachableNodes)add(HunterGoal::Patrol,node,node,.2);
  return offers;
}
bool CampaignCore::SenseHunter(const HunterCommand& sample,const HunterContext& ctx){
  if(sample.kind!=HunterOp::SeeActor&&sample.kind!=HunterOp::LoseActor)return false;
  auto c=sample;
  c.eventId="hunter_sensor";
  if(!HunterStaticValid(c)||c.atSeconds!=Now()||Paused()||Ending()!=rules::Ending::None||!ctx.physicallyVerified||!definition_->nodes.count(ctx.actualHunterNode))return false;
  if(c.kind==HunterOp::LoseActor&&c.node!=ctx.actualHunterNode)return false;
  auto next=std::make_unique<State>(*state_);
  if(ExecuteHunter(*next,c,ctx).status!=Status::Applied)return false;
  state_.swap(next);
  return true;
}
HunterResult CampaignCore::ApplyHunter(const HunterCommand& c,const HunterContext& ctx){
  if(!HunterStaticValid(c))return {Status::InvalidAction};
  const auto old=state_->receipts.find(c.eventId);
  if(old!=state_->receipts.end()){
    HunterCommand saved;
    std::optional<HunterTask> plan;
    return {detail::ParseHunter(old->second,saved,plan)&&detail::Signature(saved)==detail::Signature(c)?Status::AlreadyApplied:Status::Conflict};
  }
  if(Paused())return {Status::Paused};
  if(Ending()!=rules::Ending::None)return {Status::Finished};
  if(state_->special.phase==SpecialPhase::Collapsing ||
     (state_->special.phase==SpecialPhase::Confronting && Now()>=state_->special.boundaryAt))return {Status::AwaitingPhysics};
  if(c.atSeconds!=Now())return {Status::InvalidTime};
  if(!ctx.physicallyVerified||!definition_->nodes.count(ctx.actualHunterNode))return {Status::InvalidAction};
  if(c.kind!=HunterOp::SeeActor&&c.node!=ctx.actualHunterNode)return {Status::MissingRequirement};
  if(state_->receipts.size()+detail::CombatReservedReceipts(state_->combat) + detail::SpecialReservedReceipts(state_->special)>=detail::MaxReceipts)return {Status::LogFull};
  auto next=std::make_unique<State>(*state_);
  auto result=ExecuteHunter(*next,c,ctx);
  if(result.status!=Status::Applied)return result;
  const auto plan=c.kind==HunterOp::Choose?next->hunterRuntime.task:std::optional<HunterTask>{};
  next->receipts.emplace(c.eventId,detail::Signature(c,plan));
  state_.swap(next);
  return result;
}
HunterResult CampaignCore::ExecuteHunter(State& s,const HunterCommand& c,const HunterContext& ctx){
  if((c.atSeconds<s.combat.hunterBlockedUntil || (s.special.phase==SpecialPhase::Opportunity && c.atSeconds<s.special.boundaryAt)) &&
     (c.kind==HunterOp::Choose||c.kind==HunterOp::Arrive||c.kind==HunterOp::Complete))return {Status::MissingRequirement};
  auto& r=s.hunterRuntime;
  const auto p=HunterProfile(s.routes.DifficultyLevel());
  const HunterResult denied{Status::MissingRequirement};
  if(c.kind==HunterOp::SeeActor){
    if(!ctx.lineOfSightVerified)return denied;
    if(c.actor!="joe"){
      const auto story=s.narrative.CaptureSnapshot();
      const auto& o=story.outcomes.at(c.actor);
      if(o.dead||o.captured||o.escapedAlone||o.together)return denied;
    }
    hunter::LocationEstimate estimate{c.observedPosition,c.radiusMeters,c.atSeconds};
    if(!s.hunter.Observe({hunter::StimulusKind::VisualContact,c.actor=="joe"?hunter::Subject::Player:hunter::Subject::Companion,c.atSeconds,estimate,1,""}))return denied;
    const auto previous = r.actors.find(c.actor);
    const bool newlyPerceived = previous == r.actors.end() || !previous->second.visible || (!previous->second.threatening && c.argument != 0);
    if (r.task && ((r.task->goal == HunterGoal::Pursue && r.task->target == c.actor && r.task->destination != c.node) || (r.task->goal != HunterGoal::Pursue && newlyPerceived))) r.task.reset();
    if (newlyPerceived) r.watchingDoor.clear();
    r.actors[c.actor] = {c.node, estimate, true, c.argument != 0};
    r.latestSightings[c.actor] = detail::Signature(c);
    r.lostAt.erase(c.actor);
    return {Status::Applied};
  }
  if(c.kind==HunterOp::LoseActor){
    const auto t=r.actors.find(c.actor);
    if(t==r.actors.end()||!t->second.visible)return denied;
    t->second.visible=false;
    r.lostAt[c.actor]=c.atSeconds;
    if(c.actor=="joe"&&s.hunter.CurrentState()==hunter::State::Pursuing)s.hunter.Observe({hunter::StimulusKind::VisualLost,hunter::Subject::Player,c.atSeconds,{} ,1,""});
    if(r.task&&r.task->goal==HunterGoal::Pursue&&r.task->target==c.actor)r.task.reset();
    return {Status::Applied};
  }
  if(c.kind==HunterOp::Pressure){
    static constexpr double cooldown[]={1,3,5,15} ,amount[]={12,25,15,10};
    const auto old=r.pressureAt.find(c.argument);
    if(old!=r.pressureAt.end()&&c.atSeconds-old->second<cooldown[c.argument])return denied;
    if(c.argument==3){
      const auto t=r.actors.find(c.actor);
      if(t==r.actors.end()||t->second.visible||c.atSeconds-t->second.estimate.observedAtSeconds>s.hunter.CaptureSnapshot().tuning.locationLifetimeSeconds*(1+r.aggression/100))return denied;
    } else if(!ctx.lineOfSightVerified)return denied;
    if(!c.actor.empty()&&c.argument!=3){
      const auto t=r.actors.find(c.actor);
      if(t==r.actors.end()||!Live(t->second,c.atSeconds,p))return denied;
    }
    r.aggression=std::min(100.0,r.aggression+amount[c.argument]);
    r.pressureAt[c.argument]=c.atSeconds;
    return {Status::Applied};
  }
  if(c.kind==HunterOp::NoticePassage){
    const auto* d=Find(definition_->doors,c.target);
    const auto t=r.actors.find(c.actor);
    if(!ctx.lineOfSightVerified||t==r.actors.end()||!Live(t->second,c.atSeconds,p)||(c.node!=d->fromNode&&c.node!=d->toNode)||(t->second.node!=d->fromNode&&t->second.node!=d->toNode))return denied;
    r.passages[c.target]=c.atSeconds;
    return {Status::Applied};
  }
  if(c.kind==HunterOp::Choose){
    if(r.task)return {Status::Conflict};
    const auto offers=HunterOffers(s,ctx);
    if(offers.empty())return denied;
    if(r.serial==std::numeric_limits<std::uint64_t>::max())return {Status::LogFull};
    double total=0;
    for(const auto& o:offers)total+=o.weight;
    double draw=std::generate_canonical<double,53>(s.hunterRandom)*total;
    const auto* chosen=&offers.back();
    for(const auto& o:offers){
      draw-=o.weight;
      if(draw<0){
        chosen=&o;
        break;
      }
    }
    r.task=HunterTask{++r.serial,chosen->goal,chosen->target,chosen->destination,c.eventId,c.atSeconds,-1};
    r.watchingDoor.clear();
    return {Status::Applied,r.task};
  }
  if(!r.task||r.task->serial!=c.taskSerial||r.task->target!=c.target)return {Status::Conflict};
  auto& t=*r.task;
  if(c.kind==HunterOp::Cancel){
    r.task.reset();
    return {Status::Applied};
  }
  if(c.node!=t.destination)return denied;
  if(c.kind==HunterOp::Arrive){
    if(t.arrivedAt>=0||!ctx.reachableNodes.count(c.node))return denied;
    if((Environmental(t.goal)||t.goal==HunterGoal::WatchPassage||t.goal==HunterGoal::InspectHide)&&!ctx.reactionCueVerified)return denied;
    t.arrivedAt=c.atSeconds;
    return {Status::Applied,r.task};
  }
  if(t.arrivedAt<0)return denied;
  const double delay=Environmental(t.goal)||t.goal==HunterGoal::WatchPassage||t.goal==HunterGoal::InspectHide?p.reactionSeconds:0;
  if(c.atSeconds-t.arrivedAt<delay)return denied;
  if(Blocking(t.goal)&&!ctx.alternativePathVerified)return denied;
  world::WorldCore devices(s.world,definition_->doors,definition_->cameras,definition_->hides);
  HunterResult result{Status::Applied};
  if(Environmental(t.goal)){
    const auto* object=s.world.Find(t.target);
    const auto seen=s.evidence.CaptureSnapshot().lastSeen;
    const auto memory=seen.find(t.target);
    if(!ctx.lineOfSightVerified||!object||!object->present||memory==seen.end()||memory->second!=object->stateId)return denied;
    switch(t.goal){
      case HunterGoal::OpenDoor:result.signal=devices.OpenDoor(t.target,c.atSeconds);
      if(!result.signal)return denied;
      break;
      case HunterGoal::CloseDoor:if(!devices.CloseDoor(t.target,c.atSeconds))return denied;
      result.signal=world::WorldSignal{t.target,perception::SoundKind::Door,object->positionMeters,c.atSeconds,.35};
      break;
      case HunterGoal::PowerOff:result.signal=devices.SetCircuit(t.target,false,c.atSeconds);
      if(!result.signal)return denied;
      break;
      case HunterGoal::DisableCamera:if(!devices.SetCameraState(t.target,"interference",c.atSeconds))return denied;
      break;
      default:break;
    }
    const auto* changed=s.world.Find(t.target);
    // A device changed by Batman is his own observation, not evidence about Joe.
    s.evidence.Inspect(t.target,changed->stateId,changed->positionMeters,c.atSeconds);
  }else if(t.goal==HunterGoal::WatchPassage){
    if(!ctx.lineOfSightVerified)return denied;
    r.lastAmbushAt=c.atSeconds;
    r.watchingDoor=t.target;
  } else if(t.goal==HunterGoal::InspectHide){
    if(!ctx.lineOfSightVerified)return denied;
    result.inspectedHideOccupied=devices.Occupied(t.target);
    r.searched[t.target]=c.atSeconds;
  } else if(t.goal==HunterGoal::Investigate)r.searched[t.destination]=c.atSeconds;
  // Reaching a remembered point does not imply contact, injury or capture.
  else if(t.goal==HunterGoal::Pursue){
    const auto track=r.actors.find(t.target);
    if(track==r.actors.end()||!Live(track->second,c.atSeconds,p)||track->second.node!=t.destination)return denied;
  }
  r.task.reset();
  return result;
}
}
// namespace thug::campaign
