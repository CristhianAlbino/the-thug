#pragma once
#include "CampaignIO.h"
namespace thug::campaign::detail {
inline std::string Signature(const InteractionCommand& c, std::uint64_t order, bool accepted) {
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << "I " << order << ' ' << accepted << ' '
      << static_cast<int>(c.kind) << ' ' << std::quoted(c.actor) << ' ' << std::quoted(c.target) << ' '
      << std::quoted(c.node) << ' ' << std::quoted(c.destination) << ' ' << std::quoted(c.item) << ' '
      << c.atSeconds << ' ' << c.argument;
  return out.str();
}
inline bool ParseInteraction(const std::string& value, InteractionCommand& c, std::uint64_t& order, bool& accepted) {
  if (value.size() > 1024) return false;
  std::istringstream in(value); in.imbue(std::locale::classic());
  std::string prefix; int kind, result;
  if (!(in >> prefix >> order >> result >> kind >> std::quoted(c.actor) >> std::quoted(c.target)
        >> std::quoted(c.node) >> std::quoted(c.destination) >> std::quoted(c.item) >> c.atSeconds >> c.argument) ||
      prefix != "I" || result < 0 || result > 1 || kind < 0 || kind > static_cast<int>(InteractionOp::PerceiveThreat)) return false;
  c.kind = static_cast<InteractionOp>(kind); accepted = result != 0; in >> std::ws;
  return in.eof() && Signature(c, order, accepted) == value;
}
} // namespace thug::campaign::detail
