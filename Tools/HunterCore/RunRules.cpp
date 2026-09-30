#include "RunRules.h"

#include <cmath>
#include <stdexcept>

namespace thug::rules {

RunRules::RunRules(Difficulty difficulty) : difficulty_(difficulty) {}

void RunRules::AdvanceTo(double nowSeconds) {
  if (!std::isfinite(nowSeconds) || nowSeconds < nowSeconds_) {
    throw std::invalid_argument("Game time must be finite and monotonic");
  }
  nowSeconds_ = nowSeconds;
  if (helicopterDeadline_ && nowSeconds_ > *helicopterDeadline_ &&
      result_ == Ending::None) {
    roofFailed_ = true;
  }
}

bool RunRules::Apply(Event event, double atSeconds) {
  AdvanceTo(atSeconds);
  if (result_ != Ending::None) return false;

  switch (event) {
  case Event::SafeOpened:
    safeOpened_ = true;
    return true;
  case Event::GarageKeyTaken:
    if (!safeOpened_ || garageFailed_) return false;
    garageKey_ = true;
    return true;
  case Event::GarageGateReleased:
    if (!garageKey_ || garageFailed_) return false;
    garageGate_ = true;
    return true;
  case Event::GarageExitReached:
    if (!garageGate_ || garageFailed_) return false;
    result_ = Ending::GarageEscape;
    return true;
  case Event::RadioFound:
    radioFound_ = true;
    return true;
  case Event::BatteriesInstalled:
    if (!radioFound_ || roofFailed_) return false;
    batteriesInstalled_ = true;
    return true;
  case Event::RadioContactEstablished:
    if (!batteriesInstalled_ || roofFailed_) return false;
    radioContact_ = true;
    return true;
  case Event::HelicopterConfirmed:
    if (!radioContact_ || roofFailed_ || helicopterDeadline_) return false;
    helicopterDeadline_ = nowSeconds_ + 300; // GDD: approximately 5 minutes.
    return true;
  case Event::RoofReached:
    if (!helicopterDeadline_ || roofFailed_) return false;
    result_ = Ending::RoofEscape;
    return true;
  case Event::HelicopterDeparted:
    if (!helicopterDeadline_) return false;
    roofFailed_ = true;
    return true;
  case Event::TunnelFound:
    if (tunnelFailed_) return false;
    tunnelFound_ = true;
    return true;
  case Event::TunnelExitReached:
    if (!tunnelFound_ || tunnelFailed_) return false;
    result_ = Ending::TunnelEscape;
    return true;
  case Event::WindowFound:
    if (windowFailed_) return false;
    windowFound_ = true;
    return true;
  case Event::WindowToolsFound:
    if (windowFailed_) return false;
    windowTools_ = true;
    return true;
  case Event::WindowUnblocked:
    if (!windowFound_ || !windowTools_ || windowFailed_) return false;
    windowUnblocked_ = true;
    return true;
  case Event::WindowCrossed:
    if (!windowUnblocked_ || windowFailed_) return false;
    result_ = Ending::WindowEscape;
    return true;
  case Event::GarageBlocked:
    garageFailed_ = true;
    return true;
  case Event::TunnelIntercepted:
    tunnelFailed_ = true;
    return true;
  case Event::WindowDiscovered:
    windowFailed_ = true;
    return true;
  case Event::DawnReached:
    if (difficulty_ == Difficulty::Punitive) return false;
    result_ = Ending::SurvivedDawn;
    return true;
  case Event::HunterAbandonedSearch:
    result_ = Ending::HunterAbandoned;
    return true;
  case Event::CapturedByHunter:
    result_ = Ending::Captured;
    return true;
  case Event::JoeDied:
    result_ = Ending::Died;
    return true;
  }
  return false;
}

RouteState RunRules::Status(Route route) const {
  switch (route) {
  case Route::Garage:
    return {garageFailed_, safeOpened_ || garageKey_, garageGate_};
  case Route::Roof:
    return {roofFailed_, radioFound_, helicopterDeadline_.has_value() &&
                                      !roofFailed_};
  case Route::Tunnel:
    return {tunnelFailed_, tunnelFound_, tunnelFound_ && !tunnelFailed_};
  case Route::Window:
    return {windowFailed_, windowFound_, windowUnblocked_ && !windowFailed_};
  }
  throw std::invalid_argument("Unknown route");
}

std::optional<double> RunRules::HelicopterDeadlineSeconds() const {
  return helicopterDeadline_;
}

} // namespace thug::rules
