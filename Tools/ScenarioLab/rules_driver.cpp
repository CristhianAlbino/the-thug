#include "../HunterCore/RunRules.h"

#include <iomanip>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

using thug::rules::Difficulty;
using thug::rules::Ending;
using thug::rules::Event;
using thug::rules::Route;
using thug::rules::RunRules;

namespace {

const std::map<std::string, Event> kEvents = {
    {"SafeOpened", Event::SafeOpened},
    {"GarageKeyTaken", Event::GarageKeyTaken},
    {"GarageGateReleased", Event::GarageGateReleased},
    {"GarageExitReached", Event::GarageExitReached},
    {"RadioFound", Event::RadioFound},
    {"BatteriesInstalled", Event::BatteriesInstalled},
    {"RadioContactEstablished", Event::RadioContactEstablished},
    {"HelicopterConfirmed", Event::HelicopterConfirmed},
    {"RoofReached", Event::RoofReached},
    {"TunnelFound", Event::TunnelFound},
    {"TunnelExitReached", Event::TunnelExitReached},
    {"WindowFound", Event::WindowFound},
    {"WindowToolsFound", Event::WindowToolsFound},
    {"WindowUnblocked", Event::WindowUnblocked},
    {"WindowCrossed", Event::WindowCrossed},
    {"GarageBlocked", Event::GarageBlocked},
    {"TunnelIntercepted", Event::TunnelIntercepted},
    {"WindowDiscovered", Event::WindowDiscovered},
};

const char* EndingName(Ending ending) {
  switch (ending) {
  case Ending::None: return "None";
  case Ending::GarageEscape: return "GarageEscape";
  case Ending::RoofEscape: return "RoofEscape";
  case Ending::TunnelEscape: return "TunnelEscape";
  case Ending::WindowEscape: return "WindowEscape";
  case Ending::SurvivedDawn: return "SurvivedDawn";
  case Ending::HunterAbandoned: return "HunterAbandoned";
  case Ending::Captured: return "Captured";
  case Ending::Died: return "Died";
  case Ending::CoordinatedEscape: return "CoordinatedEscape";
  case Ending::BitterDestruction: return "BitterDestruction";
  }
  throw std::runtime_error("Unknown ending");
}

} // namespace

int main() {
  try {
    std::string difficulty;
    if (!(std::cin >> difficulty)) throw std::runtime_error("Missing difficulty");
    if (difficulty != "Hard" && difficulty != "Punitive") {
      throw std::runtime_error("Only Hard/Punitive scenarios authored");
    }
    RunRules rules(difficulty == "Hard" ? Difficulty::Hard : Difficulty::Punitive);
    std::vector<bool> accepted;
    std::string operation;
    while (std::cin >> operation) {
      double seconds;
      if (!(std::cin >> seconds)) throw std::runtime_error("Missing game time");
      if (operation == "WAIT") {
        rules.AdvanceTo(seconds);
      } else if (operation == "APPLY") {
        std::string name;
        if (!(std::cin >> name)) throw std::runtime_error("Missing event");
        const auto found = kEvents.find(name);
        if (found == kEvents.end()) throw std::runtime_error("Unknown event: " + name);
        accepted.push_back(rules.Apply(found->second, seconds));
      } else {
        throw std::runtime_error("Unknown operation: " + operation);
      }
    }
    const auto snapshot = rules.CaptureSnapshot();
    std::cout << "{\"ending\":\"" << EndingName(rules.Result())
              << "\",\"accepted\":[";
    for (std::size_t i = 0; i < accepted.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << (accepted[i] ? "true" : "false");
    }
    std::cout << "],\"failed\":{";
    const std::vector<std::pair<const char*, Route>> routes = {
        {"garage", Route::Garage}, {"roof", Route::Roof},
        {"tunnel", Route::Tunnel}, {"window", Route::Window}};
    for (std::size_t i = 0; i < routes.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << '\"' << routes[i].first << "\":"
                << (rules.Status(routes[i].second).failed ? "true" : "false");
    }
    std::cout << "},\"now\":" << std::setprecision(12) << snapshot.nowSeconds
              << ",\"deadline\":";
    if (snapshot.helicopterDeadline) std::cout << *snapshot.helicopterDeadline;
    else std::cout << "null";
    std::cout << "}\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
