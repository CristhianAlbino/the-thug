#include "RunRules.h"

#include <iostream>
#include <stdexcept>

using namespace thug::rules;

namespace {

int assertions = 0;

void Check(bool condition, const char* message) {
  ++assertions;
  if (!condition) throw std::runtime_error(message);
}

void GarageAndAlternateRoute() {
  RunRules run(Difficulty::Hard);
  Check(!run.Apply(Event::GarageExitReached, 0),
        "Cannot exit without gate and key");
  Check(!run.Apply(Event::GarageKeyTaken, 1), "Safe must be opened");
  Check(run.Apply(Event::SafeOpened, 2), "Safe opened");
  Check(run.Apply(Event::GarageKeyTaken, 3), "Key obtained");
  Check(run.Apply(Event::GarageBlocked, 4), "Garage route blocked");
  Check(!run.Apply(Event::GarageGateReleased, 5),
        "Blocked route cannot become ready");
  Check(run.Result() == Ending::None, "Route failure is not game over");
  Check(run.Apply(Event::TunnelFound, 6), "Tunnel discovered");
  Check(run.Apply(Event::TunnelExitReached, 7), "Tunnel escape works");
  Check(run.Result() == Ending::TunnelEscape, "Correct ending");
  Check(!run.Apply(Event::JoeDied, 8), "Ending is terminal");

  RunRules clean(Difficulty::Normal);
  Check(clean.Apply(Event::SafeOpened, 0) &&
            clean.Apply(Event::GarageKeyTaken, 0) &&
            clean.Apply(Event::GarageGateReleased, 0) &&
            clean.Apply(Event::GarageExitReached, 0),
        "Complete garage sequence succeeds");
  Check(clean.Result() == Ending::GarageEscape, "Garage ending");
}

void HelicopterDeadline() {
  RunRules run(Difficulty::Punitive);
  Check(!run.Apply(Event::HelicopterDeparted, 0),
        "Helicopter cannot leave before being confirmed");
  Check(!run.Apply(Event::HelicopterConfirmed, 0),
        "Cannot confirm before radio contact");
  Check(run.Apply(Event::RadioFound, 10) &&
            run.Apply(Event::BatteriesInstalled, 20) &&
            run.Apply(Event::RadioContactEstablished, 30) &&
            run.Apply(Event::HelicopterConfirmed, 40),
        "Radio sequence succeeds");
  Check(run.HelicopterDeadlineSeconds() == 340,
        "Five-minute window starts on confirmation");
  Check(!run.Apply(Event::HelicopterConfirmed, 41),
        "Cannot reset timer by confirming twice");
  run.AdvanceTo(341);
  Check(run.Status(Route::Roof).failed, "Missed helicopter closes route");
  Check(!run.Apply(Event::RoofReached, 342), "Late roof arrival fails");
  Check(run.Result() == Ending::None, "Other routes remain open");

  RunRules onTime(Difficulty::Hard);
  Check(onTime.Apply(Event::RadioFound, 0) &&
            onTime.Apply(Event::BatteriesInstalled, 0) &&
            onTime.Apply(Event::RadioContactEstablished, 0) &&
            onTime.Apply(Event::HelicopterConfirmed, 0) &&
            onTime.Apply(Event::RoofReached, 300),
        "Exact deadline still permits escape");
  Check(onTime.Result() == Ending::RoofEscape, "Roof ending");
}

void WindowAndDawn() {
  RunRules run(Difficulty::Punitive);
  Check(!run.Apply(Event::DawnReached, 18000),
        "Punitive has no automatic dawn victory");
  Check(run.Result() == Ending::None, "Punitive continues after dawn");
  Check(!run.Apply(Event::WindowCrossed, 18001),
        "Cannot cross blocked window");
  Check(run.Apply(Event::WindowFound, 18002) &&
            run.Apply(Event::WindowToolsFound, 18003) &&
            run.Apply(Event::WindowUnblocked, 18004) &&
            run.Apply(Event::WindowCrossed, 18005),
        "Window sequence succeeds after dawn");
  Check(run.Result() == Ending::WindowEscape, "Window ending");

  RunRules normal(Difficulty::Normal);
  Check(normal.Apply(Event::DawnReached, 5400),
        "Normal may win at verified dawn event");
  Check(normal.Result() == Ending::SurvivedDawn, "Dawn ending");

  RunRules hidden(Difficulty::Punitive);
  Check(hidden.Apply(Event::HunterAbandonedSearch, 10),
        "Verified Hunter abandonment is a Punitivo victory");
  Check(hidden.Result() == Ending::HunterAbandoned, "Hidden ending");
}

void ClockValidationAndFailure() {
  RunRules run(Difficulty::Easy);
  Check(run.Apply(Event::TunnelFound, 2), "Begin run");
  bool threw = false;
  try { run.AdvanceTo(1); }
  catch (const std::invalid_argument&) { threw = true; }
  Check(threw, "Time cannot go backwards");
  Check(run.Apply(Event::CapturedByHunter, 3), "Capture is a defeat");
  Check(run.Result() == Ending::Captured, "Correct defeat");
  Check(!run.Apply(Event::TunnelExitReached, 4),
        "Defeat cannot be replaced by escape");
}

} // namespace

int main() {
  try {
    GarageAndAlternateRoute();
    HelicopterDeadline();
    WindowAndDawn();
    ClockValidationAndFailure();
    std::cout << "RunRules: " << assertions << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "RunRules test failed after " << assertions
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
