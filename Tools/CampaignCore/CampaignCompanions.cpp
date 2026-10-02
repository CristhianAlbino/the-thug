#include "CampaignState.h"
#include "CompanionIO.h"
#include <algorithm>
#include <limits>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& entries,const std::string& id) {
  for(const auto& entry:entries)if(entry.id==id)return &entry;
  return nullptr;
}
bool Requires(const save::WorldState& world,const std::vector<ObjectRequirement>& requirements) {
  for(const auto& r:requirements){const auto* object=world.Find(r.id);if(!object||!object->present||object->stateId!=r.state)return false;}
  return true;
}
bool Active(const save::NarrativeSnapshot& story,const std::string& name) {
  if(name=="joe")return true;
  const auto found=story.outcomes.find(name);
  return found!=story.outcomes.end()&&!found->second.dead&&!found->second.captured&&!found->second.escapedAlone;
}
bool Present(const CompanionContext& c,const std::string& name,const std::string& node) {
  const auto found=c.actors.find(name);
  return found!=c.actors.end()&&found->second.active&&found->second.node==node;
}
bool Perceived(const CompanionContext& c,const std::string& name) {
  const auto found=c.actors.find(name);return found!=c.actors.end()&&found->second.active;
}
bool RouteObservation(const std::string& id,const std::string& node) {
  return node==detail::ExitNode(id)||(id=="garage"&&node=="G_CONTROL")||
    (id=="roof"&&node=="F1_RADIO")||(id=="tunnel"&&(node=="B1_TUNNEL"||node=="B1_PUMP"))||
    (id=="window"&&node=="F3_WINDOW");
}
bool Discover(rules::RunRules& run,const std::string& route,double at) {
  const auto state=run.CaptureSnapshot();
  if(route=="tunnel"&&!state.tunnelFound&&!state.tunnelFailed)return run.Apply(rules::Event::TunnelFound,at);
  if(route=="window"&&!state.windowFound&&!state.windowFailed)return run.Apply(rules::Event::WindowFound,at);
  return true;
}
int Ammo(const survival::Inventory& inventory) {
  int count=0;for(const auto& [id,n]:inventory.Contents())if(inventory.IsCategory(id,survival::ItemCategory::Ammunition))count+=n;
  return count;
}
void ForceFatal(survival::SurvivalCore& health) {
  auto s=health.CaptureSnapshot();s.injury=survival::Injury::Severe;s.bleeding=true;
  s.bloodLoss=s.tuning.fatalBloodLoss;s.collapsed=s.dead=true;
  if(!health.RestoreSnapshot(s))throw std::logic_error("Invalid companion death boundary");
}
}

CompanionRuntimeSnapshot CampaignCore::CompanionRuntime(companion::Character person) const {
  return state_->bodies.at(detail::Name(person)).Snapshot();
}
std::set<std::string> CampaignCore::JoeKnownRoutes() const {return state_->joeKnownRoutes;}
bool CampaignCore::PickupReserved(const State& s,const std::string& id,const std::string& except) const {
  for(const auto& [name,body]:s.bodies)if(name!=except&&body.task&&body.task->goal==companion::Goal::SearchResources&&body.task->target==id)return true;
  return false;
}
bool CampaignCore::HideReserved(const State& s,const std::string& id,const std::string& except) const {
  for(const auto& [name,body]:s.bodies)if(name!=except&&body.task&&body.task->goal==companion::Goal::StayHidden&&body.task->target==id)return true;
  return false;
}
bool CampaignCore::CompanionStaticValid(const CompanionCommand& a) const {
  if(!detail::Token(a.eventId)||!detail::Token(a.actor)||!detail::Token(a.target)||!definition_->nodes.count(a.node)||
      !std::isfinite(a.atSeconds)||a.atSeconds<0||a.kind<CompanionOp::ObservePickup||a.kind>CompanionOp::ObserveMissingPickup||
      (a.actor!="joe"&&!detail::Person(a.actor)))return false;
  if(a.actor=="joe"&&a.kind!=CompanionOp::Transfer&&a.kind!=CompanionOp::Treat&&a.kind!=CompanionOp::Stabilize&&a.kind!=CompanionOp::Abandon)return false;
  const bool task=a.kind==CompanionOp::Arrive||a.kind==CompanionOp::Complete||a.kind==CompanionOp::Cancel;
  if(task?!a.taskSerial:a.taskSerial!=0)return false;
  if(a.kind!=CompanionOp::Transfer&&a.kind!=CompanionOp::Treat&&a.kind!=CompanionOp::Stabilize&&!a.item.empty())return false;
  if(a.kind!=CompanionOp::Transfer&&a.kind!=CompanionOp::Operate&&a.quantity)return false;
  if(a.kind!=CompanionOp::BeginTask&&a.kind!=CompanionOp::Complete&&a.kind!=CompanionOp::Injury&&a.kind!=CompanionOp::Operate&&a.kind!=CompanionOp::Outcome&&a.argument)return false;
  switch(a.kind){
    case CompanionOp::ObservePickup:case CompanionOp::ObserveMissingPickup:return Find(definition_->pickups,a.target);
    case CompanionOp::ObserveHide:case CompanionOp::LeaveHide:return Find(definition_->hides,a.target);
    case CompanionOp::ObserveRoute:return bool(detail::Route(a.target))&&RouteObservation(a.target,a.node);
    case CompanionOp::ChooseTask:return a.target=="auto";
    case CompanionOp::BeginTask:return a.argument>=0&&a.argument<=static_cast<int>(companion::Goal::UseJoeAsDistraction);
    case CompanionOp::Arrive:case CompanionOp::Cancel:return true;
    case CompanionOp::Complete:return a.argument>=0&&a.argument<=static_cast<int>(companion::Goal::UseJoeAsDistraction);
    case CompanionOp::Transfer:return (a.target=="joe"||detail::Person(a.target))&&a.target!=a.actor&&a.quantity>0&&a.quantity<=100&&detail::Token(a.item)&&
      [&]{for(const auto& item:data::Items())if(item.id==a.item)return true;return false;}();
    case CompanionOp::Treat:case CompanionOp::Stabilize:return (a.target=="joe"||detail::Person(a.target))&&state_->inventory.IsCategory(a.item,survival::ItemCategory::Medical);
    case CompanionOp::Injury:return a.target==a.actor&&a.argument>=1&&a.argument<=3;
    case CompanionOp::Abandon:return a.actor=="joe"&&bool(detail::Person(a.target));
    case CompanionOp::Outcome:return a.target=="outcome"&&a.argument>=0&&a.argument<=static_cast<int>(CompanionOutcome::Together);
    case CompanionOp::Operate:{
      if(a.argument<0||a.argument>static_cast<int>(ActionKind::Circuit))return false;
      const auto kind=static_cast<ActionKind>(a.argument);
      if(kind!=ActionKind::Circuit&&a.quantity)return false;
      if(kind==ActionKind::Use)return a.target!="safe_code_accepted"&&Find(definition_->uses,a.target);
      if(kind==ActionKind::UnlockDoor)return Find(definition_->unlocks,a.target);
      if(kind==ActionKind::OpenDoor||kind==ActionKind::CloseDoor)return Find(definition_->doors,a.target);
      return kind==ActionKind::Circuit&&definition_->circuitNodes.count(a.target)&&(a.quantity==0||a.quantity==1);
    }
  }
  return false;
}
bool CampaignCore::CompanionContextValid(const CompanionCommand& a,const CompanionContext& c) const {
  if(c.actors.size()>4||c.reachableNodes.size()>definition_->nodes.size()||
      !std::isfinite(c.actualSourceMeters.x)||!std::isfinite(c.actualSourceMeters.y)||!std::isfinite(c.actualSourceMeters.z))return false;
  for(const auto& [name,actor]:c.actors)if((name!="joe"&&!detail::Person(name))||!definition_->nodes.count(actor.node))return false;
  for(const auto& node:c.reachableNodes)if(!definition_->nodes.count(node))return false;
  const auto found=c.actors.find(a.actor);
  return found!=c.actors.end()&&found->second.node==a.node&&
    (found->second.active||a.kind==CompanionOp::Outcome||a.kind==CompanionOp::Injury||a.kind==CompanionOp::Cancel);
}
std::vector<CompanionOffer> CampaignCore::Offers(const State& s,const std::string& actor,const std::string& node,const CompanionContext& c) const {
  if(!detail::Person(actor)||!Active(s.narrative.CaptureSnapshot(),actor)||!Present(c,actor,node))return {};
  const auto& body=s.bodies.at(actor);
  if(body.task||!body.hidingPlace.empty()||body.survival.CurrentEffects().dead||body.survival.CurrentEffects().collapsed)return {};
  companion::Situation situation;
  situation.hunterPerceivedNearby=c.hunterPerceived;
  situation.joePerceivedNearby=Perceived(c,"joe");
  const auto joe=c.actors.find("joe");
  const bool reachJoe=joe!=c.actors.end()&&c.reachableNodes.count(joe->second.node);
  situation.canReachJoe=reachJoe;
  situation.joeNeedsHelp=situation.joePerceivedNearby&&c.joeNeedsHelpObserved&&s.survival.CurrentInjury()!=survival::Injury::None&&body.inventory.Quantity("treatment")>0;
  std::string pickup,hide,exit;
  for(const auto& id:body.knownPickups){const auto* p=Find(definition_->pickups,id);if(p&&!PickupReserved(s,id,actor)&&c.reachableNodes.count(p->node)&&body.inventory.CanAdd(p->item,p->quantity)){pickup=id;break;}}
  for(const auto& id:body.knownHides){const auto* h=Find(definition_->hides,id);if(h&&!HideReserved(s,id,actor)&&c.reachableNodes.count(h->atNode)){hide=id;break;}}
  situation.resourcesKnownNearby=!pickup.empty();
  for(const auto& route:s.Person(*detail::Person(actor)).KnownRoutes())if(c.reachableNodes.count(detail::ExitNode(route))){situation.accessibleEscapeRoute=route;exit=route;break;}
  std::vector<CompanionOffer> offers;
  for(const auto& option:s.Person(*detail::Person(actor)).AvailableOptions(situation)){
    switch(option.goal){
      case companion::Goal::StayHidden:if(!hide.empty())offers.push_back({option.goal,option.weight,hide,Find(definition_->hides,hide)->atNode,{}});break;
      case companion::Goal::SearchResources:{const auto* p=Find(definition_->pickups,pickup);if(p)offers.push_back({option.goal,option.weight,p->id,p->node,p->item});break;}
      case companion::Goal::HelpJoe:if(reachJoe)offers.push_back({option.goal,option.weight,"joe",joe->second.node,"treatment"});break;
      case companion::Goal::ShareRoute:if(reachJoe&&option.routeId)offers.push_back({option.goal,option.weight,*option.routeId,joe->second.node,{}});break;
      case companion::Goal::EscapeAlone:if(!exit.empty())offers.push_back({option.goal,option.weight,exit,detail::ExitNode(exit),{}});break;
      case companion::Goal::UseJoeAsDistraction:if(reachJoe)offers.push_back({option.goal,option.weight,"joe",joe->second.node,{}});break;
    }
  }
  return offers;
}
std::vector<CompanionOffer> CampaignCore::CompanionOffers(const std::string& actor,const std::string& node,const CompanionContext& c) const {
  CompanionCommand a{"query",CompanionOp::ChooseTask,actor,"auto",node,Now()};
  if(Paused()||Ending()!=rules::Ending::None||!CompanionContextValid(a,c))return {};
  return Offers(*state_,actor,node,c);
}

CompanionResult CampaignCore::ApplyCompanion(const CompanionCommand& a,const CompanionContext& c) {
  if(!CompanionStaticValid(a))return {Status::InvalidAction};
  const auto signature=detail::Signature(a);const auto receipt=state_->receipts.find(a.eventId);
  if(receipt!=state_->receipts.end())return {receipt->second==signature?Status::AlreadyApplied:Status::Conflict};
  if(Paused())return {Status::Paused};
  if(a.atSeconds!=Now())return {Status::InvalidTime};
  if(!a.physicallyVerified||!CompanionContextValid(a,c))return {Status::InvalidAction};
  if(Ending()!=rules::Ending::None&&a.kind!=CompanionOp::Outcome&&a.kind!=CompanionOp::Cancel)return {Status::Finished};
  if(state_->receipts.size()+detail::CombatReservedReceipts(state_->combat)>=detail::MaxReceipts)return {Status::LogFull};
  auto next=std::make_unique<State>(*state_);
  const auto result=ExecuteCompanion(*next,a,c);
  if(result.status!=Status::Applied)return result;
  next->receipts.emplace(a.eventId,signature);next->SyncEnding();ReconcileCombat(*next);
  state_.swap(next);return result;
}

CompanionResult CampaignCore::ExecuteCompanion(State& s,const CompanionCommand& a,const CompanionContext& c) {
  const CompanionResult denied{Status::MissingRequirement};
  const auto actor=detail::Person(a.actor);
  auto story=s.narrative.CaptureSnapshot();
  if((!Active(story,a.actor)||(actor&&story.outcomes.at(a.actor).together))&&a.kind!=CompanionOp::Cancel)return denied;
  auto& source=a.actor=="joe"?s.inventory:s.bodies.at(a.actor).inventory;
  auto& health=a.actor=="joe"?s.survival:s.bodies.at(a.actor).survival;
  CompanionBody* body=actor?&s.bodies.at(a.actor):nullptr;
  if(body&&!body->hidingPlace.empty()&&(a.node!=Find(definition_->hides,body->hidingPlace)->atNode||
     (a.kind!=CompanionOp::LeaveHide&&a.kind!=CompanionOp::Injury&&a.kind!=CompanionOp::Outcome&&a.kind!=CompanionOp::Cancel)))return denied;
  if(a.actor=="joe"&&!s.hidingPlace.empty()&&a.node!=Find(definition_->hides,s.hidingPlace)->atNode)return denied;
  if((health.CurrentEffects().dead||health.CurrentEffects().collapsed)&&a.kind!=CompanionOp::Cancel&&a.kind!=CompanionOp::Outcome&&a.kind!=CompanionOp::Injury)return denied;
  if(body&&body->task&&a.kind!=CompanionOp::Arrive&&a.kind!=CompanionOp::Complete&&a.kind!=CompanionOp::Cancel&&a.kind!=CompanionOp::Injury&&a.kind!=CompanionOp::Outcome)return denied;
  world::WorldCore devices(s.world,definition_->doors,definition_->cameras,definition_->hides);
  const auto syncAmmo=[&](const std::string& name)->bool{
    if(name=="joe")return true;
    auto& person=s.Person(*detail::Person(name));companion::Influence delta;
    delta.ammunition=Ammo(s.bodies.at(name).inventory)-person.CurrentTraits().ammunition;
    return delta.ammunition==0||person.ApplyInfluence(delta,a.eventId,a.atSeconds);
  };
  const auto syncHealth=[&](const std::string& name){if(name!="joe")s.Person(*detail::Person(name)).SetInjured(s.bodies.at(name).survival.CurrentInjury()!=survival::Injury::None,a.atSeconds);};
  const auto releaseHide=[&](const std::string& name){auto& b=s.bodies.at(name);if(!b.hidingPlace.empty()){if(!devices.LeaveHide(b.hidingPlace,a.atSeconds))return false;b.hidingPlace.clear();}return true;};
  const auto reachableRecipient=[&](const std::string& name){
    if(!Present(c,name,a.node)||!Active(story,name))return false;
    if(name=="joe")return s.hidingPlace.empty()||Find(definition_->hides,s.hidingPlace)->atNode==a.node;
    const auto& recipient=s.bodies.at(name);
    return !story.outcomes.at(name).together&&(recipient.hidingPlace.empty()||Find(definition_->hides,recipient.hidingPlace)->atNode==a.node);
  };
  CompanionResult result{Status::Applied};
  switch(a.kind){
    case CompanionOp::ObservePickup:{const auto* p=Find(definition_->pickups,a.target);const auto* object=s.world.Find(a.target);if(a.node!=p->node||!object||!object->present||object->stateId!="available"||!body->knownPickups.insert(a.target).second)return denied;break;}
    case CompanionOp::ObserveMissingPickup:{const auto* p=Find(definition_->pickups,a.target);const auto* object=s.world.Find(a.target);if(a.node!=p->node||!object||object->present||object->stateId!="collected"||!body->knownPickups.erase(a.target))return denied;break;}
    case CompanionOp::ObserveHide:{const auto* h=Find(definition_->hides,a.target);if(a.node!=h->atNode||!s.world.Find(a.target)->present||!body->knownHides.insert(a.target).second)return denied;break;}
    case CompanionOp::ObserveRoute:if(!s.Person(*actor).LearnRoute(a.target,a.atSeconds))return denied;break;
    case CompanionOp::ChooseTask:case CompanionOp::BeginTask:{
      const auto offers=Offers(s,a.actor,a.node,c);if(offers.empty()||body->serial==std::numeric_limits<std::uint64_t>::max())return denied;
      const CompanionOffer* selected=nullptr;
      if(a.kind==CompanionOp::BeginTask){for(const auto& offer:offers)if(static_cast<int>(offer.goal)==a.argument&&offer.target==a.target){selected=&offer;break;}}
      else {
        // Choose among executable offers only, preserving the person's weighted
        // policy/RNG. Missing physical targets are not replaced by phantom acts.
        const auto choice=s.Person(*actor).ChooseAmong([&]{std::vector<companion::Option> options;for(const auto& offer:offers)options.push_back({offer.goal,offer.weight,offer.goal==companion::Goal::ShareRoute||offer.goal==companion::Goal::EscapeAlone?std::optional<std::string>{offer.target}:std::nullopt});return options;}());
        for(const auto& offer:offers)if(offer.goal==choice.goal&&(!choice.routeId||offer.target==*choice.routeId)){selected=&offer;break;}
      }
      if(!selected||(selected->goal==companion::Goal::SearchResources&&PickupReserved(s,selected->target))||
          (selected->goal==companion::Goal::StayHidden&&HideReserved(s,selected->target)))return denied;
      body->task=CompanionTask{++body->serial,selected->goal,selected->target,selected->destination,selected->item,TaskPhase::Travelling,a.atSeconds,-1,a.eventId};
      body->lastStop=TaskStop::None;result.task=body->task;break;
    }
    case CompanionOp::Arrive:case CompanionOp::Complete:case CompanionOp::Cancel:{
      if(!body->task||a.taskSerial!=body->task->serial||a.target!=body->task->target)return denied;
      const auto task=*body->task;
      if(a.kind==CompanionOp::Cancel){body->Stop(TaskStop::Cancelled);break;}
      if(a.node!=task.destination)return denied;
      if(a.kind==CompanionOp::Arrive){if(task.phase!=TaskPhase::Travelling||!c.movementCompleted||!c.reachableNodes.count(a.node))return denied;body->task->phase=TaskPhase::Ready;body->task->readyAt=a.atSeconds;result.task=body->task;break;}
      if(a.argument!=static_cast<int>(task.goal)||task.phase!=TaskPhase::Ready||!c.actionCompleted||a.atSeconds<task.readyAt+partydata::InteractionSeconds[static_cast<int>(task.goal)])return denied;
      switch(task.goal){
        case companion::Goal::StayHidden:if(HideReserved(s,task.target,a.actor)||!devices.EnterHide(task.target,a.atSeconds))return denied;body->hidingPlace=task.target;break;
        case companion::Goal::SearchResources:{const auto* p=Find(definition_->pickups,task.target);auto object=*s.world.Find(p->id);if(!object.present||object.stateId!="available"||!Requires(s.world,p->requirements)||!source.Add(p->item,p->quantity))return denied;
          if(p->onCollect&&!s.routes.Apply(*p->onCollect,a.atSeconds))return denied;
          object.stateId="collected";object.present=false;if(!s.world.SetObject(p->id,object,a.atSeconds)||!syncAmmo(a.actor))return denied;body->knownPickups.erase(p->id);break;}
        case companion::Goal::HelpJoe:if(!Present(c,"joe",a.node)||!source.Remove(task.item,1,survival::RemovalReason::Consume)||!s.survival.Treat())return denied;break;
        case companion::Goal::ShareRoute:if(!Present(c,"joe",a.node)||!s.Person(*actor).KnownRoutes().count(task.target)||!Discover(s.routes,task.target,a.atSeconds))return denied;s.joeKnownRoutes.insert(task.target);break;
        case companion::Goal::EscapeAlone:if(!s.Person(*actor).KnownRoutes().count(task.target)||!Discover(s.routes,task.target,a.atSeconds)||!s.routes.Status(*detail::Route(task.target)).ready||s.routes.Status(*detail::Route(task.target)).failed)return denied;
          story.outcomes.at(a.actor).escapedAlone=true;story.outcomes.at(a.actor).lastContactNode=a.node;if(!s.narrative.RestoreSnapshot(story))return denied;break;
        case companion::Goal::UseJoeAsDistraction:{
          if(!Present(c,"joe",a.node)||!c.hunterPerceived)return denied;
          companion::Situation situation;situation.hunterPerceivedNearby=situation.joePerceivedNearby=true;
          const auto options=s.Person(*actor).AvailableOptions(situation);
          if(std::none_of(options.begin(),options.end(),[](const auto& o){return o.goal==companion::Goal::UseJoeAsDistraction;}))return denied;
          if(c.joeWitnessedAction&&a.actor=="bartholomeu"){story.durableFacts.insert("barth_betrayal_executed");if(!s.narrative.RestoreSnapshot(story))return denied;}
          result.actorSignal=CompanionSignal{a.actor,a.node,perception::SoundKind::Distraction,c.actualSourceMeters,a.atSeconds};break;}
      }
      body->Stop(TaskStop::Completed);break;
    }
    case CompanionOp::Transfer:{
      if(!c.actionCompleted||!reachableRecipient(a.target)||
          (a.target!="joe"&&s.bodies.at(a.target).task))return denied;
      auto& target=a.target=="joe"?s.inventory:s.bodies.at(a.target).inventory;
      if(a.actor=="joe"&&!CombatItemAvailable(s,a.item,a.quantity))return denied;
      if(a.actor=="joe"&&a.item=="rifle"&&(s.combat.loadedRounds||s.combat.task))return denied;
      if(!source.TransferTo(target,a.item,a.quantity))return denied;
      if(a.actor=="joe"&&a.item=="magazine"){
        companion::Influence delta{20,0,0,10,0,0,a.quantity};if(!s.Person(*detail::Person(a.target)).ApplyInfluence(delta,a.eventId,a.atSeconds))return denied;
        story.durableFacts.insert("joe_shared_ammo_with_"+a.target);if(!s.narrative.RestoreSnapshot(story))return denied;
      }else if(!syncAmmo(a.target))return denied;
      if(!syncAmmo(a.actor))return denied;
      break;
    }
    case CompanionOp::Treat:case CompanionOp::Stabilize:{
      if(!c.actionCompleted||!reachableRecipient(a.target))return denied;
      auto& target=a.target=="joe"?s.survival:s.bodies.at(a.target).survival;
      if(!source.Remove(a.item,1,survival::RemovalReason::Consume)||!(a.kind==CompanionOp::Treat?target.Treat():target.Stabilize()))return denied;
      syncHealth(a.target);
      if(a.actor=="joe"&&a.target!="joe"){
        if(!s.Person(*detail::Person(a.target)).ApplyInfluence(partydata::MedicalHelp[static_cast<int>(*detail::Person(a.target))],a.eventId,a.atSeconds))return denied;
        story.durableFacts.insert("joe_shared_treatment_with_"+a.target);if(!s.narrative.RestoreSnapshot(story))return denied;
      }
      break;
    }
    case CompanionOp::Injury:if(!health.ApplyInjury(static_cast<survival::Injury>(a.argument)))return denied;body->Stop(TaskStop::Injured);syncHealth(a.actor);break;
    case CompanionOp::LeaveHide:if(body->hidingPlace!=a.target||!c.actionCompleted||!releaseHide(a.actor))return denied;break;
    case CompanionOp::Abandon:if(!c.actionCompleted||!Perceived(c,a.target)||!Active(story,a.target)||!s.Person(*detail::Person(a.target)).ApplyExperience(companion::Experience::JoeAbandonedCompanion,a.eventId,a.atSeconds))return denied;
      if(a.target=="harley"){story.durableFacts.insert("joe_abandoned_harley");if(!s.narrative.RestoreSnapshot(story))return denied;}break;
    case CompanionOp::Outcome:{
      if(!c.actionCompleted||story.outcomes.at(a.actor).together)return denied;
      auto& outcome=story.outcomes.at(a.actor);
      const auto kind=static_cast<CompanionOutcome>(a.argument);
      if(kind==CompanionOutcome::Together){if(!story.joeEscaped||!Present(c,"joe",a.node))return denied;outcome.together=true;}
      else if(kind==CompanionOutcome::Captured)outcome.captured=true;
      else {outcome.dead=true;ForceFatal(health);syncHealth(a.actor);}
      outcome.lastContactNode=a.node;if(!s.narrative.RestoreSnapshot(story)||(kind!=CompanionOutcome::Dead&&!releaseHide(a.actor)))return denied;
      body->Stop(TaskStop::Outcome);break;
    }
    case CompanionOp::Operate:{
      if(!c.actionCompleted)return denied;
      const auto kind=static_cast<ActionKind>(a.argument);
      if(kind==ActionKind::Use){const auto* u=Find(definition_->uses,a.target);auto object=*s.world.Find(u->object);
        if(a.node!=u->node||!object.present||object.stateId!=u->before||!Requires(s.world,u->requirements)||(!u->item.empty()&&source.Quantity(u->item)<u->quantity))return denied;
        if(u->consume&&!source.Remove(u->item,u->quantity,survival::RemovalReason::Consume))return denied;
        if(u->runEvent&&!s.routes.Apply(*u->runEvent,a.atSeconds))return denied;
        object.stateId=u->after;if(!s.world.SetObject(u->object,object,a.atSeconds)||!syncAmmo(a.actor))return denied;
      }else if(kind==ActionKind::UnlockDoor){const auto* u=Find(definition_->unlocks,a.target);if(a.node!=u->node||source.Quantity(u->item)<u->quantity||!devices.UnlockDoor(u->door,a.atSeconds))return denied;if(u->consume&&!source.Remove(u->item,u->quantity,survival::RemovalReason::Consume))return denied;if(!syncAmmo(a.actor))return denied;
      }else if(kind==ActionKind::Circuit){if(a.node!=definition_->circuitNodes.at(a.target))return denied;result.worldSignal=devices.SetCircuit(a.target,a.quantity!=0,a.atSeconds);if(!result.worldSignal)return denied;
      }else {const auto* door=Find(definition_->doors,a.target);if(a.node!=door->fromNode&&a.node!=door->toNode)return denied;if(kind==ActionKind::OpenDoor){result.worldSignal=devices.OpenDoor(a.target,a.atSeconds);if(!result.worldSignal)return denied;}else if(!devices.CloseDoor(a.target,a.atSeconds))return denied;}
      break;
    }
  }
  if(body)body->lastNode=a.node;
  return result;
}

void CampaignCore::AdvanceCompanions(State& s,double elapsed,const std::map<std::string,survival::Activity>& activities) {
  auto story=s.narrative.CaptureSnapshot();
  for(auto& [name,body]:s.bodies){
    if(!Active(story,name)||story.outcomes.at(name).together)continue;
    const auto before=body.survival.CaptureSnapshot();double dt=elapsed;bool fatal=false;
    if(before.bleeding){const double remaining=(before.tuning.fatalBloodLoss-before.bloodLoss)/before.tuning.bleedingPerSecond;if(remaining<=dt){dt=remaining;fatal=true;}}
    const auto activity=activities.find(name);const auto result=body.survival.Advance(dt,activity==activities.end()?survival::Activity::Rest:activity->second);
    if(fatal&&!result.becameDead)ForceFatal(body.survival);
    s.Person(*detail::Person(name)).SetInjured(body.survival.CurrentInjury()!=survival::Injury::None,s.world.NowSeconds());
    if(body.survival.CurrentEffects().dead){story.outcomes.at(name).dead=true;story.outcomes.at(name).lastContactNode=body.lastNode;body.Stop(TaskStop::Outcome);}
    else if(body.survival.CurrentEffects().collapsed)body.Stop(TaskStop::Collapsed);
  }
  if(!s.narrative.RestoreSnapshot(story))throw std::logic_error("Invalid individual survival outcomes");
}
} // namespace thug::campaign
