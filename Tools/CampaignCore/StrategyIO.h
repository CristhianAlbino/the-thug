#pragma once
#include "CampaignIO.h"
#include "CombatIO.h"

namespace thug::campaign::detail {
inline std::string Signature(const StrategyCommand& c, std::uint64_t order, bool accepted,
    std::uint64_t actorHarm, std::uint64_t peerHarm) {
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << "Q " << order << ' '
      << static_cast<int>(c.kind) << ' ' << std::quoted(c.actor) << ' ' << std::quoted(c.target) << ' '
      << std::quoted(c.node) << ' ' << std::quoted(c.item) << ' ' << std::quoted(c.destination) << ' '
      << std::quoted(c.reference) << ' ' << c.atSeconds << ' ' << c.argument << ' ' << c.serial << ' ';
  WriteVector(out,c.position); out << ' ' << accepted << ' ' << actorHarm << ' ' << peerHarm;
  return out.str();
}
inline bool ParseStrategy(const std::string& value, StrategyCommand& c, std::uint64_t& order,
    bool& accepted, std::uint64_t& actorHarm, std::uint64_t& peerHarm) {
  std::istringstream in(value); in.imbue(std::locale::classic());
  std::string tag; int kind, yes;
  if (value.size()>2048 || !(in >> tag >> order >> kind >> std::quoted(c.actor) >> std::quoted(c.target)
      >> std::quoted(c.node) >> std::quoted(c.item) >> std::quoted(c.destination) >> std::quoted(c.reference)
      >> c.atSeconds >> c.argument >> c.serial) || tag!="Q" || kind<0 ||
      kind>static_cast<int>(StrategyOp::CancelPlan) || !ReadVector(in,c.position) ||
      !(in >> yes >> actorHarm >> peerHarm) || yes<0 || yes>1) return false;
  c.kind=static_cast<StrategyOp>(kind); accepted=yes!=0; in>>std::ws;
  return in.eof() && Signature(c,order,accepted,actorHarm,peerHarm)==value;
}
inline std::string StrategyFingerprint(const StrategyRuntimeSnapshot& r) {
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << r.serial;
  for(const auto& [id,d]:r.drops) {
    out << " D " << std::quoted(id) << std::quoted(d.owner) << std::quoted(d.item) << std::quoted(d.node) << ' ' << d.quantity << ' ';
    WriteVector(out,d.position); for(const auto& a:d.knownTo) out << std::quoted(a);
  }
  for(const auto& [actor,nodes]:r.observations) for(const auto& [node,o]:nodes)
    out << " O " << std::quoted(actor) << std::quoted(node) << std::quoted(o.eventId) << ' ' << o.atSeconds << ' ' << o.hunterPresent;
  for(const auto& [actor,drops]:r.knownDrops) for(const auto& [id,d]:drops) {
    out << " KD " << std::quoted(actor) << std::quoted(id) << std::quoted(d.item) << std::quoted(d.node)
      << ' ' << d.quantity << ' ' << d.atSeconds << ' '; WriteVector(out,d.position);
  }
  for(const auto& [id,p]:r.reports) out << " P " << std::quoted(id) << std::quoted(p.source) << std::quoted(p.recipient)
    << std::quoted(p.node) << std::quoted(p.route) << ' ' << p.atSeconds << ' ' << p.hunterPresent
    << ' ' << p.deliberateLie << ' ' << p.checked << ' ' << p.contradicted;
  for(const auto* task:{&r.question,&r.joint}) {
    out << " T " << bool(*task);
    if(*task) { const auto& t=**task;
      out << ' ' << t.serial << ' ' << t.actorHarm << ' ' << t.peerHarm << ' ' << static_cast<int>(t.kind)
        << std::quoted(t.actor) << std::quoted(t.peer) << std::quoted(t.target) << std::quoted(t.node)
        << std::quoted(t.startEventId) << ' ' << t.startedAt << ' ' << t.readyAt << ' ' << t.interrupted;
    }
  }
  for(const auto& [serial,node]:r.seenSmoke) out << " SM " << serial << std::quoted(node);
  for(const auto& [node,p]:r.smokePatterns) out << " SP " << std::quoted(node) << ' ' << p.uses << ' ' << p.lastAt;
  out << " HP " << r.lastPlanAt << ' ' << bool(r.hunterPlan);
  if(r.hunterPlan) { const auto& p=*r.hunterPlan;
    out << ' ' << p.serial << ' ' << p.harm << ' ' << static_cast<int>(p.kind) << std::quoted(p.target)
      << std::quoted(p.destination) << std::quoted(p.auxiliary) << std::quoted(p.startEventId) << ' ' << p.step
      << ' ' << p.startedAt << ' ' << p.arrivedAt << ' ' << p.interrupted;
  }
  return Hash(out.str());
}
} // namespace thug::campaign::detail
