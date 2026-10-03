#pragma once
#include "CampaignIO.h"
namespace thug::campaign::detail {
inline std::string Signature(const RescueCommand& c, std::uint64_t order,
    bool witnessed, std::uint64_t helperHarm, std::uint64_t targetHarm) {
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << "R " << order << ' '
      << static_cast<int>(c.kind) << ' ' << std::quoted(c.actor) << ' ' << std::quoted(c.target) << ' '
      << std::quoted(c.node) << ' ' << std::quoted(c.item) << ' ' << c.atSeconds << ' ' << c.serial
      << ' ' << witnessed << ' ' << helperHarm << ' ' << targetHarm;
  return out.str();
}
inline bool ParseRescue(const std::string& value, RescueCommand& c, std::uint64_t& order,
    bool& witnessed, std::uint64_t& helperHarm, std::uint64_t& targetHarm) {
  std::istringstream in(value); in.imbue(std::locale::classic());
  std::string prefix; int kind, seen;
  if (value.size() > 1024 || !(in >> prefix >> order >> kind >> std::quoted(c.actor)
      >> std::quoted(c.target) >> std::quoted(c.node) >> std::quoted(c.item) >> c.atSeconds
      >> c.serial >> seen >> helperHarm >> targetHarm) || prefix != "R" || kind < 0 ||
      kind > static_cast<int>(RescueOp::FinalizeCapture) || seen < 0 || seen > 1) return false;
  c.kind = static_cast<RescueOp>(kind); witnessed = seen != 0; in >> std::ws;
  return in.eof() && Signature(c, order, witnessed, helperHarm, targetHarm) == value;
}
inline void WriteRescueRuntime(std::ostream& out, const RescueRuntimeSnapshot& r) {
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << "RESCUE " << r.serial << ' ' << r.captures.size() << '\n';
  for (const auto& [actor, t] : r.captures) {
    out << "CAPTURE " << std::quoted(actor) << ' ' << static_cast<int>(t.phase) << ' ' << std::quoted(t.node)
        << ' ' << std::quoted(t.startEventId) << ' ' << t.serial << ' ' << t.startedAt << ' ' << t.containedAt << ' ' << bool(t.release) << '\n';
    if (t.release) { const auto& a = *t.release;
      out << "RELEASE " << a.serial << ' ' << a.helperHarm << ' ' << a.targetHarm << ' ' << std::quoted(a.helper)
          << ' ' << std::quoted(a.item) << ' ' << std::quoted(a.startEventId) << ' ' << a.startedAt << ' '
          << a.readyAt << ' ' << a.self << ' ' << a.interrupted << '\n';
    }
  }
}
} // namespace thug::campaign::detail
