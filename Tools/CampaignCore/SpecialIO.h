#pragma once
#include "CampaignIO.h"
namespace thug::campaign::detail {
inline std::string SpecialSignature(const SpecialCommand& c, std::uint64_t order) {
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << "S " << static_cast<int>(c.kind) << ' ' << std::quoted(c.target) << ' '
      << std::quoted(c.node) << ' ' << c.atSeconds << ' ' << c.serial << ' ' << c.argument
      << ' ' << c.fates.size();
  for (const auto& [actor, fate] : c.fates) out << ' ' << std::quoted(actor) << ' ' << static_cast<int>(fate);
  out << " ORDER " << order;
  return out.str();
}
inline bool ParseSpecial(const std::string& signature, SpecialCommand& c, std::uint64_t& order) {
  std::istringstream in(signature); in.imbue(std::locale::classic());
  std::string tag; int kind; std::size_t count;
  if (!(in >> tag >> kind >> std::quoted(c.target) >> std::quoted(c.node) >> c.atSeconds >> c.serial >> c.argument >> count) ||
      tag != "S" || kind < 0 || kind > 10 || count > 3) return false;
  c.kind = static_cast<SpecialOp>(kind);
  for (std::size_t i = 0; i < count; ++i) {
    std::string actor; int fate;
    if (!(in >> std::quoted(actor) >> fate) || fate < 0 || fate > 3 ||
        !c.fates.emplace(actor, static_cast<SpecialFate>(fate)).second) return false;
  }
  if (!(in >> tag >> order) || tag != "ORDER") return false;
  in >> std::ws;
  return in.eof() && SpecialSignature(c, order) == signature;
}
} // namespace thug::campaign::detail
