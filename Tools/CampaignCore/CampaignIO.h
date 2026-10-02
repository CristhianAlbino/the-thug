#pragma once

#include "CampaignCore.h"
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace thug::campaign::detail {
inline constexpr std::size_t MaxReceipts = 10000;
inline constexpr std::size_t MaxRuntimeBytes = 4 * 1024 * 1024;

// Keep completion receipts available for work already committed/prepared.
// Shots can expire; a live grenade must be resolvable at its fuse boundary.
inline std::size_t CombatReservedReceipts(const CombatRuntimeSnapshot& r) {
  return r.thrown.size() + (r.task ? (r.task->kind == CombatOp::BeginThrow ? 2 : 1) : 0);
}

inline bool Token(const std::string& value) {
  if (value.empty() || value.size() > 128) return false;
  for (unsigned char c : value)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ':' || c == '.'))
      return false;
  return true;
}

inline std::string Hash(const std::string& value) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : value) { hash ^= c; hash *= 1099511628211ULL; }
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

inline std::string Signature(const Action& action) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << static_cast<int>(action.kind) << ' ' << std::quoted(action.target) << ' '
      << std::quoted(action.node) << ' ' << action.atSeconds << ' ' << action.argument
      << ' ' << action.presentationSerial << ' ' << std::quoted(action.cueId);
  return out.str();
}

inline bool ParseSignature(const std::string& value, Action& action) {
  if (value.size() > 1024) return false;
  std::istringstream in(value);
  in.imbue(std::locale::classic());
  int kind;
  if (!(in >> kind >> std::quoted(action.target) >> std::quoted(action.node)
           >> action.atSeconds >> action.argument >> action.presentationSerial >> std::quoted(action.cueId)) ||
      kind < 0 || kind > static_cast<int>(ActionKind::AdvanceAct)) return false;
  action.kind = static_cast<ActionKind>(kind);
  in >> std::ws;
  return in.eof() && Signature(action) == value;
}
} // namespace thug::campaign::detail
