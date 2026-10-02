#pragma once
#include "CampaignIO.h"
namespace thug::campaign::detail {
inline void WriteHunterTask(std::ostream& out,const HunterTask& t){
  out<<t.serial<<' '<<static_cast<int>(t.goal)<<' '<<std::quoted(t.target)<<' '<<std::quoted(t.destination)<<' '<<std::quoted(t.startEventId)<<' '<<t.startedAt<<' '<<t.arrivedAt;
}
inline bool ReadHunterTask(std::istream& in,HunterTask& t){
  int goal;
  if(!(in>>t.serial>>goal>>std::quoted(t.target)>>std::quoted(t.destination)>>std::quoted(t.startEventId)>>t.startedAt>>t.arrivedAt)||goal<0||goal>8)return false;
  t.goal=static_cast<HunterGoal>(goal);
  return true;
}
inline std::string Signature(const HunterCommand& c,const std::optional<HunterTask>& plan={}){
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out<<std::setprecision(std::numeric_limits<double>::max_digits10)<<"H "<<static_cast<int>(c.kind)<<' '<<std::quoted(c.actor)<<' '<<std::quoted(c.target)<<' '<<std::quoted(c.node)<<' '<<c.atSeconds<<' '<<c.taskSerial<<' '<<c.argument<<' '<<c.observedPosition.x<<' '<<c.observedPosition.y<<' '<<c.observedPosition.z<<' '<<c.radiusMeters<<' '<<bool(plan);
  if(plan){
    out<<' ';
    WriteHunterTask(out,*plan);
  }
  return out.str();
}
inline bool ParseHunter(const std::string& value,HunterCommand& c,std::optional<HunterTask>& plan){
  if(value.size()>1024)return false;
  std::istringstream in(value);
  in.imbue(std::locale::classic());
  std::string prefix;
  int kind,hasPlan;
  if(!(in>>prefix>>kind>>std::quoted(c.actor)>>std::quoted(c.target)>>std::quoted(c.node)>>c.atSeconds>>c.taskSerial>>c.argument>>c.observedPosition.x>>c.observedPosition.y>>c.observedPosition.z>>c.radiusMeters>>hasPlan)||prefix!="H"||kind<0||kind>7||hasPlan<0||hasPlan>1)return false;
  c.kind=static_cast<HunterOp>(kind);
  plan.reset();
  if(hasPlan){
    HunterTask t;
    if(!ReadHunterTask(in,t))return false;
    plan=std::move(t);
  }
  in>>std::ws;
  return in.eof()&&Signature(c,plan)==value;
}
inline bool Actor(const std::string& id){
  return id=="joe"||id=="harley"||id=="bartholomeu"||id=="jack";
}
inline bool Finite(hunter::Vec3 p){
  return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
inline double Distance(hunter::Vec3 a,hunter::Vec3 b){
  return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);
}
}
// namespace thug::campaign::detail
