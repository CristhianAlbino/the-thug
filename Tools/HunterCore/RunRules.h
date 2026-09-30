#pragma once

#include <optional>

namespace thug::rules {

enum class Difficulty { Easy, Normal, Hard, Punitive };
enum class Ending {
  None, GarageEscape, RoofEscape, TunnelEscape, WindowEscape,
  SurvivedDawn, HunterAbandoned, Captured, Died
};
enum class Route { Garage, Roof, Tunnel, Window };
enum class Event {
  SafeOpened, GarageKeyTaken, GarageGateReleased, GarageExitReached,
  RadioFound, BatteriesInstalled, RadioContactEstablished,
  HelicopterConfirmed, RoofReached, HelicopterDeparted,
  TunnelFound, TunnelExitReached,
  WindowFound, WindowToolsFound, WindowUnblocked, WindowCrossed,
  GarageBlocked, TunnelIntercepted, WindowDiscovered,
  DawnReached, HunterAbandonedSearch, CapturedByHunter, JoeDied
};

struct RouteState {
  bool failed = false;
  bool discovered = false;
  bool ready = false;
};

struct RunSnapshot {
  Difficulty difficulty;
  double nowSeconds = 0;
  Ending result = Ending::None;
  bool safeOpened = false;
  bool garageKey = false;
  bool garageGate = false;
  bool garageFailed = false;
  bool radioFound = false;
  bool batteriesInstalled = false;
  bool radioContact = false;
  bool roofFailed = false;
  std::optional<double> helicopterDeadline;
  bool tunnelFound = false;
  bool tunnelFailed = false;
  bool windowFound = false;
  bool windowTools = false;
  bool windowUnblocked = false;
  bool windowFailed = false;
};

// The world verifies interactions and emits events. These rules guard their
// sequence and decide results; they never open a door or spawn a helicopter.
class RunRules {
public:
  explicit RunRules(Difficulty difficulty);

  // Game time must be finite and monotonic. Invalid time throws.
  void AdvanceTo(double nowSeconds);
  // Returns false for a missing prerequisite, failed route or finished run.
  bool Apply(Event event, double atSeconds);
  Ending Result() const { return result_; }
  RouteState Status(Route route) const;
  std::optional<double> HelicopterDeadlineSeconds() const;
  Difficulty DifficultyLevel() const { return difficulty_; }
  RunSnapshot CaptureSnapshot() const;
  bool RestoreSnapshot(const RunSnapshot& snapshot);

private:
  Difficulty difficulty_;
  double nowSeconds_ = 0;
  Ending result_ = Ending::None;
  bool safeOpened_ = false;
  bool garageKey_ = false;
  bool garageGate_ = false;
  bool garageFailed_ = false;
  bool radioFound_ = false;
  bool batteriesInstalled_ = false;
  bool radioContact_ = false;
  bool roofFailed_ = false;
  std::optional<double> helicopterDeadline_;
  bool tunnelFound_ = false;
  bool tunnelFailed_ = false;
  bool windowFound_ = false;
  bool windowTools_ = false;
  bool windowUnblocked_ = false;
  bool windowFailed_ = false;
};

} // namespace thug::rules
