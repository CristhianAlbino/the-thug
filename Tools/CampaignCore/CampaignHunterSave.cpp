#include "CampaignState.h"
#include "HunterIO.h"
#include <algorithm>
namespace thug::campaign {
namespace {
bool Tag(std::istream& in,const char* expected){
  std::string tag;
  return bool(in>>tag)&&tag==expected;
}
bool Time(double time,double now){
  return std::isfinite(time)&&time>=0&&time<=now;
}
bool SamePlan(const HunterTask& a,const HunterTask& b){
  return a.serial==b.serial&&a.goal==b.goal&&a.target==b.target&&a.destination==b.destination&&a.startEventId==b.startEventId&&a.startedAt==b.startedAt;
}
}
void CampaignCore::WriteHunter(std::ostream& out,const State& s)const{
  const auto& r=s.hunterRuntime;
  out<<std::setprecision(std::numeric_limits<double>::max_digits10);
  out<<"HUNTER_RUNTIME "<<r.nowSeconds<<' '<<r.aggression<<' '<<r.lastAmbushAt<<' '<<r.serial<<' '<<std::quoted(r.watchingDoor)<<'\n';
  out<<"HUNTER_RNG "<<std::quoted(([&]{
    std::ostringstream rng;
    rng<<s.hunterRandom;
    return rng.str();
  })())<<'\n';
  out<<"TRACKS "<<r.actors.size()<<'\n';
  for(const auto& [actor,t]:r.actors)out<<"TRACK "<<std::quoted(actor)<<' '<<std::quoted(t.node)<<' '<<t.estimate.center.x<<' '<<t.estimate.center.y<<' '<<t.estimate.center.z<<' '<<t.estimate.radiusMeters<<' '<<t.estimate.observedAtSeconds<<' '<<t.visible<<' '<<t.threatening<<'\n';
  out<<"SIGHTINGS "<<r.latestSightings.size()<<'\n';
  for(const auto& [actor,sample]:r.latestSightings)out<<std::quoted(actor)<<' '<<std::quoted(sample)<<'\n';
  const auto times=[&](const char* tag,const auto& values){
    out<<tag<<' '<<values.size()<<'\n';
    for(const auto& [id,time]:values)out<<std::quoted(id)<<' '<<time<<'\n';
  };
  times("LOST",r.lostAt);
  times("PASSAGES",r.passages);
  times("SEARCHED",r.searched);
  out<<"PRESSURE "<<r.pressureAt.size()<<'\n';
  for(const auto& [kind,time]:r.pressureAt)out<<kind<<' '<<time<<'\n';
  out<<"HUNTER_TASK "<<bool(r.task)<<'\n';
  if(r.task){
    detail::WriteHunterTask(out,*r.task);
    out<<'\n';
  }
}
bool CampaignCore::ReadHunter(std::istream& in,State& s)const{
  HunterRuntimeSnapshot r;
  std::string rng;
  if(!Tag(in,"HUNTER_RUNTIME")||!(in>>r.nowSeconds>>r.aggression>>r.lastAmbushAt>>r.serial>>std::quoted(r.watchingDoor))||!Tag(in,"HUNTER_RNG")||!(in>>std::quoted(rng))||rng.size()>10000)return false;
  std::istringstream random(rng);
  std::mt19937_64 engine;
  if(!(random>>engine))return false;
  random>>std::ws;
  if(!random.eof())return false;
  std::size_t count;
  if(!Tag(in,"TRACKS")||!(in>>count)||count>4)return false;
  for(std::size_t i=0; i<count; ++i){
    std::string actor;
    HunterTrack t;
    int visible,threat;
    if(!Tag(in,"TRACK")||!(in>>std::quoted(actor)>>std::quoted(t.node)>>t.estimate.center.x>>t.estimate.center.y>>t.estimate.center.z>>t.estimate.radiusMeters>>t.estimate.observedAtSeconds>>visible>>threat)||visible<0||visible>1||threat<0||threat>1)return false;
    t.visible=visible!=0;
    t.threatening=threat!=0;
    if(!r.actors.emplace(actor,t).second)return false;
  }
  if(!Tag(in,"SIGHTINGS")||!(in>>count)||count>4)return false;
  for(std::size_t i=0; i<count; ++i){
    std::string actor,sample;
    if(!(in>>std::quoted(actor)>>std::quoted(sample))||sample.size()>1024||!r.latestSightings.emplace(actor,sample).second)return false;
  }
  const auto times=[&](const char* tag,auto& values,std::size_t max){
    std::size_t n;
    if(!Tag(in,tag)||!(in>>n)||n>max)return false;
    for(std::size_t i=0;i<n;++i){
      std::string id;
      double time;
      if(!(in>>std::quoted(id)>>time)||!values.emplace(id,time).second)return false;
    }
    return true;
  };
  if(!times("LOST",r.lostAt,4)||!times("PASSAGES",r.passages,definition_->doors.size())||!times("SEARCHED",r.searched,definition_->nodes.size()+definition_->hides.size())||!Tag(in,"PRESSURE")||!(in>>count)||count>4)return false;
  for(std::size_t i=0; i<count; ++i){
    int kind;
    double time;
    if(!(in>>kind>>time)||!r.pressureAt.emplace(kind,time).second)return false;
  }
  int task;
  if(!Tag(in,"HUNTER_TASK")||!(in>>task)||task<0||task>1)return false;
  if(task){
    HunterTask t;
    if(!detail::ReadHunterTask(in,t))return false;
    r.task=std::move(t);
  }
  s.hunterRuntime=std::move(r);
  s.hunterRandom=engine;
  return true;
}
bool CampaignCore::HunterCoherent(const State& s)const{
  const auto& r=s.hunterRuntime;
  const double now=s.world.NowSeconds();
  const auto profile=HunterProfile(s.routes.DifficultyLevel());
  if(r.nowSeconds!=now||!std::isfinite(r.aggression)||r.aggression<0||r.aggression>100||r.lastAmbushAt < -1||(!Time(r.lastAmbushAt,now)&&r.lastAmbushAt!=-1))return false;
  std::map<std::uint64_t,HunterTask> plans;
  std::map<std::uint64_t,std::vector<HunterCommand>> taskCommands;
  std::vector<HunterCommand> pressure;
  std::map<std::string,double> passages,searched;
  std::string lastWatch;
  double lastAmbush=-1;
  for(const auto& [id,signature]:s.receipts)if(signature.rfind("H ",0)==0){
    HunterCommand c;
    c.eventId=id;
    std::optional<HunterTask> plan;
    if(!detail::ParseHunter(signature,c,plan)||!HunterStaticValid(c)||c.atSeconds>now||bool(plan)!=(c.kind==HunterOp::Choose))return false;
    if(plan){
      if(!plan->serial||plan->startedAt!=c.atSeconds||plan->arrivedAt!=-1||plan->startEventId!=id||!detail::Token(plan->target)||!definition_->nodes.count(plan->destination)||!plans.emplace(plan->serial,*plan).second)return false;
    }
    if(c.kind==HunterOp::Pressure)pressure.push_back(c);
    if(c.kind==HunterOp::NoticePassage)passages[c.target]=std::max(passages[c.target],c.atSeconds);
    if(c.taskSerial)taskCommands[c.taskSerial].push_back(c);
  }
  if(plans.size()!=r.serial||passages!=r.passages)return false;
  std::uint64_t serial=0;
  for(const auto& [n,plan]:plans){
    if(n!=++serial)return false;
    switch(plan.goal){
      case HunterGoal::Pursue:if(!detail::Actor(plan.target))return false;
      break;
      case HunterGoal::Investigate:case HunterGoal::Patrol:if(plan.target!=plan.destination)return false;
      break;
      case HunterGoal::InspectHide:{bool valid=false;
      for(const auto& h:definition_->hides)if(h.id==plan.target&&h.atNode==plan.destination)valid=true;
      if(!valid)return false;
      break;
      } case HunterGoal::BreakWedge:case HunterGoal::WatchPassage:case HunterGoal::CloseDoor:case HunterGoal::OpenDoor:{bool valid=false;
      for(const auto& d:definition_->doors)if(d.id==plan.target&&(d.fromNode==plan.destination||d.toNode==plan.destination))valid=true;
      if(!valid)return false;
      break;
      } case HunterGoal::PowerOff:{const auto n=definition_->circuitNodes.find(plan.target);
      if(n==definition_->circuitNodes.end()||n->second!=plan.destination)return false;
      break;
      } case HunterGoal::DisableCamera:{bool valid=false;
      for(const auto& c:definition_->cameras)if(c.id==plan.target&&c.atNode==plan.destination)valid=true;
      if(!valid)return false;
      break;
      }
    }
    bool arrived=false,terminal=false;
    double at=-1;
    for(const auto& c:taskCommands[n])if(c.kind==HunterOp::Arrive){
      if(arrived||c.target!=plan.target||c.node!=plan.destination||c.atSeconds<plan.startedAt)return false;
      arrived=true;
      at=c.atSeconds;
    }
    for(const auto& c:taskCommands[n])if(c.kind!=HunterOp::Arrive){
      if(terminal||c.target!=plan.target||c.atSeconds<plan.startedAt)return false;
      terminal=true;
      if(c.kind==HunterOp::Complete){
        double delay=(plan.goal>=HunterGoal::InspectHide&&plan.goal<=HunterGoal::DisableCamera)||plan.goal==HunterGoal::BreakWedge?profile.reactionSeconds:0;
        if(plan.goal==HunterGoal::BreakWedge){bool bound=false;for(const auto& w:definition_->wedges)if(w.id==plan.target){delay+=w.resistanceSeconds;bound=true;}if(!bound)return false;}
        if(!arrived||c.node!=plan.destination||c.atSeconds-at<delay)return false;
        if(plan.goal==HunterGoal::Investigate||plan.goal==HunterGoal::InspectHide)searched[plan.target]=std::max(searched[plan.target],c.atSeconds);
        if(plan.goal==HunterGoal::WatchPassage&&c.atSeconds>=lastAmbush){
          lastAmbush=c.atSeconds;
          lastWatch=plan.target;
        }
      }
    }
    if(r.task&&r.task->serial==n){
      if(terminal||!SamePlan(*r.task,plan)||r.task->arrivedAt!=at)return false;
    }
  }
  for(const auto& [n,commands]:taskCommands){
    (void)commands;
    if(!plans.count(n))return false;
  }
  if(r.searched!=searched||r.lastAmbushAt!=lastAmbush||(!r.watchingDoor.empty()&&(r.watchingDoor!=lastWatch||now-lastAmbush>=profile.watchSeconds||s.routes.Result()!=rules::Ending::None)))return false;
  if(r.task&&(!r.task->serial||r.task->serial!=r.serial||!plans.count(r.task->serial)||s.routes.Result()!=rules::Ending::None))return false;
  for(const auto& [actor,t]:r.actors){
    if(!detail::Actor(actor)||!definition_->nodes.count(t.node)||!detail::Finite(t.estimate.center)||!std::isfinite(t.estimate.radiusMeters)||t.estimate.radiusMeters<0||!Time(t.estimate.observedAtSeconds,now)||(t.visible&&now-t.estimate.observedAtSeconds>profile.visualLease))return false;
    const auto source=r.latestSightings.find(actor);
    if(source==r.latestSightings.end())return false;
    HunterCommand c;
    c.eventId="hunter_sensor";
    std::optional<HunterTask> plan;
    if(!detail::ParseHunter(source->second,c,plan)||plan||!HunterStaticValid(c)||c.kind!=HunterOp::SeeActor||c.actor!=actor||c.atSeconds!=t.estimate.observedAtSeconds||c.node!=t.node||c.observedPosition.x!=t.estimate.center.x||c.observedPosition.y!=t.estimate.center.y||c.observedPosition.z!=t.estimate.center.z||c.radiusMeters!=t.estimate.radiusMeters||bool(c.argument)!=t.threatening)return false;
    const auto loss=r.lostAt.find(actor);
    const bool lost=loss!=r.lostAt.end()&&loss->second>=t.estimate.observedAtSeconds&&Time(loss->second,now);
    bool inactive=false;
    if(actor!="joe"){
      const auto story=s.narrative.CaptureSnapshot();
      const auto& o=story.outcomes.at(actor);
      inactive=o.dead||o.captured||o.escapedAlone||o.together;
    }
    if((!t.visible&&!lost&&!inactive&&now-t.estimate.observedAtSeconds<=profile.visualLease))return false;
  }
  if(r.latestSightings.size()!=r.actors.size())return false;
  for(const auto& [actor,time]:r.lostAt)if(!r.actors.count(actor)||!Time(time,now)||r.actors.at(actor).visible||time<r.actors.at(actor).estimate.observedAtSeconds)return false;
  if(r.task&&r.task->goal==HunterGoal::Pursue){
    const auto t=r.actors.find(r.task->target);
    if(t==r.actors.end()||!t->second.visible||t->second.node!=r.task->destination)return false;
  }
  std::sort(pressure.begin(),pressure.end(),[](const auto& a,const auto& b){
    return std::tie(a.atSeconds,a.argument)<std::tie(b.atSeconds,b.argument);
  });
  std::map<int,double> pressureAt;
  double aggression=0,previous=0;
  static constexpr double cooldown[]={1,3,5,15} ,amount[]={12,25,15,10};
  for(const auto& c:pressure){
    const auto old=pressureAt.find(c.argument);
    if(old!=pressureAt.end()&&c.atSeconds-old->second<cooldown[c.argument])return false;
    aggression=std::max(0.0,aggression-(c.atSeconds-previous)*profile.aggressionDecay);
    aggression=std::min(100.0,aggression+amount[c.argument]);
    previous=c.atSeconds;
    pressureAt[c.argument]=c.atSeconds;
  }
  aggression=std::max(0.0,aggression-(now-previous)*profile.aggressionDecay);
  return pressureAt==r.pressureAt&&std::abs(aggression-r.aggression)<1e-8;
}
}
// namespace thug::campaign
