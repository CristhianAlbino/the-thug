#include "CampaignState.h"
#include "CompanionIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
bool Tag(std::istream& in,const char* expected) {std::string tag;return bool(in>>tag)&&tag==expected;}
bool Quoted(std::istream& in,std::string& value,bool empty=false) {
  in>>std::ws;return in.peek()=='"'&&bool(in>>std::quoted(value))&&value.size()<=128&&(empty||detail::Token(value));
}
void WriteSet(std::ostream& out,const char* tag,const std::set<std::string>& entries) {
  out<<tag<<' '<<entries.size()<<'\n';for(const auto& id:entries)out<<std::quoted(id)<<'\n';
}
bool ReadSet(std::istream& in,const char* tag,std::set<std::string>& entries,std::size_t maximum) {
  std::size_t count;if(!Tag(in,tag)||!(in>>count)||count>maximum)return false;
  entries.clear();for(std::size_t i=0;i<count;++i){std::string id;if(!Quoted(in,id)||!entries.insert(id).second)return false;}return true;
}
template<class T> const T* Find(const std::vector<T>& entries,const std::string& id) {
  for(const auto& entry:entries)if(entry.id==id)return &entry;
  return nullptr;
}
}
void CampaignCore::WriteCompanions(std::ostream& out,const State& s) const {
  out<<std::setprecision(std::numeric_limits<double>::max_digits10)<<"PARTY 3\n";
  for(const auto& [name,body]:s.bodies){
    out<<"BODY "<<std::quoted(name)<<' '<<std::quoted(body.lastNode)<<' '<<std::quoted(body.hidingPlace)<<' '<<body.serial<<' '<<static_cast<int>(body.lastStop)<<'\n';
    const auto inventory=body.inventory.CaptureSnapshot();out<<"BAG "<<inventory.contents.size()<<'\n';
    for(const auto& [id,n]:inventory.contents)out<<std::quoted(id)<<' '<<n<<'\n';
    const auto h=body.survival.CaptureSnapshot();
    out<<"HEALTH "<<static_cast<int>(h.injury)<<' '<<h.stamina<<' '<<h.bloodLoss<<' '<<h.bleeding<<' '<<h.exhausted<<' '<<h.collapsed<<' '<<h.dead<<'\n';
    WriteSet(out,"PICKUPS",body.knownPickups);WriteSet(out,"HIDES",body.knownHides);
    out<<"TASK "<<bool(body.task)<<'\n';
    if(body.task){const auto& t=*body.task;out<<"INTENT "<<t.serial<<' '<<static_cast<int>(t.goal)<<' '<<std::quoted(t.target)<<' '<<std::quoted(t.destination)<<' '<<std::quoted(t.item)<<' '<<static_cast<int>(t.phase)<<' '<<t.startedAt<<' '<<t.readyAt<<' '<<std::quoted(t.startEventId)<<'\n';}
  }
  WriteSet(out,"JOE_ROUTES",s.joeKnownRoutes);
}
bool CampaignCore::ReadCompanions(std::istream& in,State& s) const {
  std::size_t count;if(!Tag(in,"PARTY")||!(in>>count)||count!=3)return false;
  std::set<std::string> names;
  for(std::size_t i=0;i<count;++i){
    std::string name,node,hide;std::uint64_t serial;int stop;
    if(!Tag(in,"BODY")||!Quoted(in,name)||!s.bodies.count(name)||!names.insert(name).second||!Quoted(in,node,true)||!Quoted(in,hide,true)||!(in>>serial>>stop)||stop<0||stop>static_cast<int>(TaskStop::CampaignEnded))return false;
    auto& b=s.bodies.at(name);auto inventory=b.inventory.CaptureSnapshot();inventory.contents.clear();
    std::size_t items;if(!Tag(in,"BAG")||!(in>>items)||items>inventory.definitions.size())return false;
    for(std::size_t j=0;j<items;++j){std::string id;int n;if(!Quoted(in,id)||!(in>>n)||n<=0||!inventory.contents.emplace(id,n).second)return false;}
    if(!b.inventory.RestoreSnapshot(inventory))return false;
    auto h=b.survival.CaptureSnapshot();int injury,bleeding,exhausted,collapsed,dead;
    if(!Tag(in,"HEALTH")||!(in>>injury>>h.stamina>>h.bloodLoss>>bleeding>>exhausted>>collapsed>>dead)||injury<0||injury>3||bleeding<0||bleeding>1||exhausted<0||exhausted>1||collapsed<0||collapsed>1||dead<0||dead>1)return false;
    h.injury=static_cast<survival::Injury>(injury);h.bleeding=bleeding!=0;h.exhausted=exhausted!=0;h.collapsed=collapsed!=0;h.dead=dead!=0;
    if(!b.survival.RestoreSnapshot(h)||!ReadSet(in,"PICKUPS",b.knownPickups,definition_->pickups.size())||!ReadSet(in,"HIDES",b.knownHides,definition_->hides.size()))return false;
    int hasTask;if(!Tag(in,"TASK")||!(in>>hasTask)||hasTask<0||hasTask>1)return false;
    b.task.reset();if(hasTask){CompanionTask t;int goal,phase;if(!Tag(in,"INTENT")||!(in>>t.serial>>goal)||goal<0||goal>5||!Quoted(in,t.target)||!Quoted(in,t.destination)||!Quoted(in,t.item,true)||!(in>>phase>>t.startedAt>>t.readyAt)||phase<0||phase>1||!Quoted(in,t.startEventId))return false;t.goal=static_cast<companion::Goal>(goal);t.phase=static_cast<TaskPhase>(phase);b.task=std::move(t);}
    b.lastNode=std::move(node);b.hidingPlace=std::move(hide);b.serial=serial;b.lastStop=static_cast<TaskStop>(stop);
  }
  return ReadSet(in,"JOE_ROUTES",s.joeKnownRoutes,4);
}
bool CampaignCore::CompanionCoherent(const State& s) const {
  const auto story=s.narrative.CaptureSnapshot();const double now=s.world.NowSeconds();
  std::map<std::string,std::set<std::string>> hideProof,routeProof;
  std::map<std::string,std::map<std::string,int>> pickupBalance;
  std::map<std::string,std::uint64_t> starts;
  std::map<std::string,std::vector<CompanionCommand>> commands;
  std::set<std::string> joeRoutes,medicalFacts,capturedProof,togetherProof,escapedProof,deathProof;
  bool abandonedHarley=false,barthDistraction=false;
  for(const auto& [id,signature]:s.receipts){
    if(signature.rfind("C ",0)!=0){Action a;if(detail::ParseSignature(signature,a)&&a.kind==ActionKind::Trigger){const auto* trigger=Find(definition_->triggers,a.target);if(trigger&&trigger->event==rules::Event::TunnelFound)joeRoutes.insert("tunnel");if(trigger&&trigger->event==rules::Event::WindowFound)joeRoutes.insert("window");}continue;}
    CompanionCommand a;a.eventId=id;if(!detail::ParseCompanion(signature,a)||!CompanionStaticValid(a)||a.atSeconds>now)return false;
    commands[a.actor].push_back(a);
    if(a.kind==CompanionOp::ObservePickup){const auto* p=Find(definition_->pickups,a.target);if(a.node!=p->node)return false;++pickupBalance[a.actor][a.target];}
    if(a.kind==CompanionOp::Complete&&a.argument==static_cast<int>(companion::Goal::SearchResources))--pickupBalance[a.actor][a.target];
    if(a.kind==CompanionOp::ObserveMissingPickup){const auto* p=Find(definition_->pickups,a.target);if(a.node!=p->node)return false;--pickupBalance[a.actor][a.target];}
    if(a.kind==CompanionOp::ObserveHide){const auto* h=Find(definition_->hides,a.target);if(a.node!=h->atNode)return false;hideProof[a.actor].insert(a.target);}
    if(a.kind==CompanionOp::ObserveRoute)routeProof[a.actor].insert(a.target);
    if(a.kind==CompanionOp::BeginTask||a.kind==CompanionOp::ChooseTask)++starts[a.actor];
    if(a.kind==CompanionOp::Complete&&a.argument==static_cast<int>(companion::Goal::ShareRoute))joeRoutes.insert(a.target);
    if((a.kind==CompanionOp::Treat||a.kind==CompanionOp::Stabilize)&&a.actor=="joe")medicalFacts.insert("joe_shared_treatment_with_"+a.target);
    if(a.kind==CompanionOp::Outcome){
      if(a.argument==static_cast<int>(CompanionOutcome::Captured))capturedProof.insert(a.actor);
      if(a.argument==static_cast<int>(CompanionOutcome::Together))togetherProof.insert(a.actor);
      if(a.argument==static_cast<int>(CompanionOutcome::Dead))deathProof.insert(a.actor);
    }
    if(a.kind==CompanionOp::Injury&&a.argument==static_cast<int>(survival::Injury::Severe))deathProof.insert(a.actor);
    if(a.kind==CompanionOp::Complete&&a.argument==static_cast<int>(companion::Goal::EscapeAlone))escapedProof.insert(a.actor);
    if(a.kind==CompanionOp::Abandon&&a.target=="harley")abandonedHarley=true;
    if(a.kind==CompanionOp::Complete&&a.actor=="bartholomeu"&&a.argument==static_cast<int>(companion::Goal::UseJoeAsDistraction))barthDistraction=true;
  }
  if((story.durableFacts.count("barth_betrayal_executed")&&!barthDistraction)||joeRoutes!=s.joeKnownRoutes||bool(story.durableFacts.count("joe_abandoned_harley"))!=abandonedHarley)return false;
  for(const auto& name:{"harley","bartholomeu","jack"})if(bool(story.durableFacts.count(std::string("joe_shared_treatment_with_")+name))!=bool(medicalFacts.count(std::string("joe_shared_treatment_with_")+name)))return false;
  std::set<std::string> occupied,reservedPickups,reservedHides;
  if(!s.hidingPlace.empty())occupied.insert(s.hidingPlace);
  for(const auto& [name,b]:s.bodies){
    std::set<std::string> pickups;for(const auto& [id,balance]:pickupBalance[name]){if(balance<0||balance>1)return false;if(balance==1)pickups.insert(id);}
    const auto h=b.survival.CaptureSnapshot();const auto& person=s.Person(*detail::Person(name));const auto outcome=story.outcomes.at(name);
    const bool inactive=outcome.dead||outcome.captured||outcome.escapedAlone||outcome.together;
    if(outcome.captured!=bool(capturedProof.count(name))||outcome.together!=bool(togetherProof.count(name))||
        outcome.escapedAlone!=bool(escapedProof.count(name))||(outcome.dead&&!deathProof.count(name))||
        (inactive&&outcome.lastContactNode!=b.lastNode))return false;
    int ammo=0;for(const auto& [id,n]:b.inventory.Contents())if(b.inventory.IsCategory(id,survival::ItemCategory::Ammunition))ammo+=n;
    if(person.CurrentTraits().ammunition!=ammo||person.CurrentTraits().injured!=(h.injury!=survival::Injury::None)||h.dead!=outcome.dead||
        (!b.lastNode.empty()&&!definition_->nodes.count(b.lastNode))||b.serial!=starts[name]||
        b.knownPickups!=pickups||b.knownHides!=hideProof[name]||person.KnownRoutes()!=routeProof[name]||
        (!b.serial&&b.lastStop!=TaskStop::None))return false;
    if(!b.hidingPlace.empty()){const auto* hide=Find(definition_->hides,b.hidingPlace);const auto* object=s.world.Find(b.hidingPlace);if(!hide||!b.knownHides.count(hide->id)||b.lastNode!=hide->atNode||!object||!object->present||object->stateId!="occupied"||!occupied.insert(hide->id).second)return false;}
    if(b.task){
      const auto& t=*b.task;
      if(inactive||h.collapsed||!b.hidingPlace.empty()||s.routes.Result()!=rules::Ending::None||b.lastStop!=TaskStop::None||!t.serial||t.serial!=b.serial||
          !std::isfinite(t.startedAt)||t.startedAt<0||t.startedAt>now||!std::isfinite(t.readyAt)||!definition_->nodes.count(t.destination))return false;
      const auto start=s.receipts.find(t.startEventId);CompanionCommand began;
      if(start==s.receipts.end()||!detail::ParseCompanion(start->second,began)||began.actor!=name||began.atSeconds!=t.startedAt||
          (began.kind!=CompanionOp::BeginTask&&began.kind!=CompanionOp::ChooseTask)||
          (began.kind==CompanionOp::BeginTask&&(began.argument!=static_cast<int>(t.goal)||began.target!=t.target)))return false;
      if(t.phase==TaskPhase::Travelling&&b.lastNode!=began.node)return false;
      bool arrived=false;
      for(const auto& a:commands[name])if(a.taskSerial==t.serial){
        if(a.kind==CompanionOp::Complete||a.kind==CompanionOp::Cancel)return false;
        if(a.kind==CompanionOp::Arrive){if(arrived||a.target!=t.target||a.node!=t.destination||a.atSeconds!=t.readyAt)return false;arrived=true;}
      }
      if(t.phase==TaskPhase::Travelling){if(arrived||t.readyAt!=-1)return false;}
      else if(!arrived||t.readyAt<t.startedAt||t.readyAt>now||b.lastNode!=t.destination)return false;
      switch(t.goal){
        case companion::Goal::StayHidden:{const auto* hide=Find(definition_->hides,t.target);if(!hide||!b.knownHides.count(t.target)||t.destination!=hide->atNode||!t.item.empty()||!reservedHides.insert(t.target).second)return false;break;}
        case companion::Goal::SearchResources:{const auto* p=Find(definition_->pickups,t.target);if(!p||!b.knownPickups.count(t.target)||t.destination!=p->node||t.item!=p->item||!b.inventory.CanAdd(p->item,p->quantity)||!reservedPickups.insert(t.target).second)return false;break;}
        case companion::Goal::HelpJoe:if(t.target!="joe"||t.item!="treatment"||b.inventory.Quantity(t.item)<1||h.injury!=survival::Injury::None)return false;break;
        case companion::Goal::ShareRoute:if(!person.KnownRoutes().count(t.target)||!t.item.empty())return false;break;
        case companion::Goal::EscapeAlone:if(s.routes.DifficultyLevel()==rules::Difficulty::Easy||!person.KnownRoutes().count(t.target)||t.destination!=detail::ExitNode(t.target)||!t.item.empty())return false;break;
        case companion::Goal::UseJoeAsDistraction:if((s.routes.DifficultyLevel()!=rules::Difficulty::Hard&&s.routes.DifficultyLevel()!=rules::Difficulty::Punitive)||t.target!="joe"||!t.item.empty())return false;break;
      }
    }else if(b.serial&&b.lastStop==TaskStop::None)return false;
  }
  for(const auto& hide:definition_->hides){const auto* object=s.world.Find(hide.id);if(!object||bool(occupied.count(hide.id))!=(object->stateId=="occupied"))return false;}
  for(const auto& route:s.joeKnownRoutes)if(!detail::Route(route))return false;
  return true;
}
} // namespace thug::campaign
