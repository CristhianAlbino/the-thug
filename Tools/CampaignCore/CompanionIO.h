#pragma once
#include "CampaignIO.h"

namespace thug::campaign::detail {
inline std::optional<companion::Character> Person(const std::string& name) {
  if (name == "harley") return companion::Character::Harley;
  if (name == "bartholomeu") return companion::Character::Bartholomeu;
  if (name == "jack") return companion::Character::Jack;
  return {};
}
inline const char* Name(companion::Character person) {
  switch (person) {
    case companion::Character::Harley: return "harley";
    case companion::Character::Bartholomeu: return "bartholomeu";
    case companion::Character::Jack: return "jack";
  }
  throw std::invalid_argument("Unknown companion");
}
inline std::optional<rules::Route> Route(const std::string& id) {
  if (id == "garage") return rules::Route::Garage;
  if (id == "roof") return rules::Route::Roof;
  if (id == "tunnel") return rules::Route::Tunnel;
  if (id == "window") return rules::Route::Window;
  return {};
}
inline std::string ExitNode(const std::string& route) {
  if (route == "garage") return "X_GARAGE";
  if (route == "roof") return "X_ROOF";
  if (route == "tunnel") return "X_TUNNEL";
  if (route == "window") return "X_WINDOW";
  return {};
}
inline std::string Signature(const CompanionCommand& a) {
  std::ostringstream out;out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << "C " << static_cast<int>(a.kind) << ' ' << std::quoted(a.actor) << ' '
      << std::quoted(a.target) << ' ' << std::quoted(a.node) << ' ' << a.atSeconds
      << ' ' << std::quoted(a.item) << ' ' << a.quantity << ' ' << a.taskSerial << ' ' << a.argument;
  return out.str();
}
inline bool ParseCompanion(const std::string& value, CompanionCommand& a) {
  if (value.size() > 1024) return false;
  std::istringstream in(value);in.imbue(std::locale::classic());std::string prefix;int kind;
  if (!(in >> prefix >> kind >> std::quoted(a.actor) >> std::quoted(a.target)
      >> std::quoted(a.node) >> a.atSeconds >> std::quoted(a.item) >> a.quantity
      >> a.taskSerial >> a.argument) || prefix != "C" || kind < 0 || kind > static_cast<int>(CompanionOp::ObserveMissingPickup)) return false;
  a.kind = static_cast<CompanionOp>(kind);in >> std::ws;
  return in.eof() && Signature(a) == value;
}
} // namespace thug::campaign::detail
