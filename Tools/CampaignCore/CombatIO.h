#pragma once
#include "CampaignIO.h"

namespace thug::campaign::detail {
inline bool CombatFinite(hunter::Vec3 p) {
  return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
inline double CombatDistance(hunter::Vec3 a, hunter::Vec3 b) {
  return std::hypot(a.x - b.x, a.y - b.y, a.z - b.z);
}
inline void WriteVector(std::ostream& out, hunter::Vec3 p) { out << p.x << ' ' << p.y << ' ' << p.z; }
inline bool ReadVector(std::istream& in, hunter::Vec3& p) { return bool(in >> p.x >> p.y >> p.z); }
inline std::string Signature(const CombatCommand& c) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << "W " << static_cast<int>(c.kind) << ' ' << std::quoted(c.node) << ' '
      << std::quoted(c.target) << ' ' << c.atSeconds << ' ' << c.serial << ' ';
  WriteVector(out, c.position); out << ' '; WriteVector(out, c.vector);
  out << ' ' << std::quoted(c.actor) << ' ' << c.contacts.size();
  for (const auto& h : c.contacts) {
    out << ' ' << std::quoted(h.target) << ' ' << std::quoted(h.node) << ' ';
    WriteVector(out, h.position); out << ' ' << static_cast<int>(h.injury);
  }
  return out.str();
}
inline std::string Receipt(const CombatCommand& c, std::uint64_t order, std::uint64_t created) {
  return Signature(c) + " ORDER " + std::to_string(order) + " CREATED " + std::to_string(created);
}
inline bool ParseCombat(const std::string& signature, CombatCommand& c,
    std::uint64_t& order, std::uint64_t& created) {
  if (signature.size() > 4096) return false;
  std::istringstream in(signature);
  in.imbue(std::locale::classic());
  std::string tag;
  int kind;
  std::size_t count;
  if (!(in >> tag >> kind >> std::quoted(c.node) >> std::quoted(c.target) >> c.atSeconds >> c.serial) ||
      tag != "W" || kind < 0 || kind > static_cast<int>(CombatOp::ReleaseBreath) ||
      !ReadVector(in, c.position) || !ReadVector(in, c.vector) || !(in >> std::quoted(c.actor) >> count) || count > 16) return false;
  c.kind = static_cast<CombatOp>(kind);
  c.contacts.clear();
  for (std::size_t i = 0; i < count; ++i) {
    BlastContact hit;
    int severity;
    if (!(in >> std::quoted(hit.target) >> std::quoted(hit.node)) || !ReadVector(in, hit.position) ||
        !(in >> severity) || severity < 0 || severity > 3) return false;
    hit.injury = static_cast<survival::Injury>(severity);
    c.contacts.push_back(std::move(hit));
  }
  if (!(in >> tag) || tag != "ORDER" || !(in >> order >> tag) || tag != "CREATED" || !(in >> created)) return false;
  in >> std::ws;
  return in.eof() && Receipt(c, order, created) == signature;
}
} // namespace thug::campaign::detail
