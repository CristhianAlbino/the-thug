#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace thug::companion {

enum class Character { Harley, Bartholomeu, Jack };
enum class Difficulty { Easy, Normal, Hard, Punitive };
enum class Experience { JoeSharedAmmo, JoeAbandonedCompanion };
enum class Goal {
  StayHidden, SearchResources, HelpJoe, ShareRoute,
  EscapeAlone, UseJoeAsDistraction
};

// 0..100; initial values are supplied by authored character data, not invented here.
struct Traits {
  double trust;
  double fear;
  double stress;
  double loyalty;
  double selfPreservation;
  double relationship;
  int ammunition = 0;
  bool injured = false;
};

struct Influence {
  double trust = 0;
  double fear = 0;
  double stress = 0;
  double loyalty = 0;
  double selfPreservation = 0;
  double relationship = 0;
  int ammunition = 0;
};

struct Situation {
  bool hunterPerceivedNearby = false;
  bool joePerceivedNearby = false;
  bool joeNeedsHelp = false;
  bool canReachJoe = false;
  bool resourcesKnownNearby = false;
  std::optional<std::string> accessibleEscapeRoute;
};

struct Option {
  Goal goal;
  double weight;
  std::optional<std::string> routeId;
};

// Own knowledge is separate from what Joe and the Hunter know. The world layer
// must validate events and spatial reachability before calling this core.
class CompanionCore {
public:
  CompanionCore(Character character, Difficulty difficulty, Traits initial,
                std::uint64_t seed = 1);

  Character Identity() const { return character_; }
  const Traits& CurrentTraits() const { return traits_; }
  const std::set<std::string>& KnownRoutes() const { return knownRoutes_; }

  // Applies the two GDD-specified numerical consequences, once per actual act.
  // Read-only validation for coordinators that must commit another resource.
  bool CanApplyExperience(Experience experience, const std::string& eventId,
                          double atSeconds) const;
  bool ApplyExperience(Experience experience, const std::string& eventId,
                       double atSeconds);
  // Only call when the character actually learned a route by observation/dialogue.
  bool LearnRoute(const std::string& routeId, double atSeconds);
  // Authoring layer may apply other documented events with bounded deltas.
  bool ApplyInfluence(const Influence& delta, const std::string& eventId,
                      double atSeconds);
  void SetInjured(bool injured, double atSeconds);

  std::vector<Option> AvailableOptions(const Situation& situation) const;
  Option Choose(const Situation& situation);

private:
  static bool ValidTraits(const Traits& traits);
  static double Clamp(double value);
  void AdvanceTo(double atSeconds);

  Character character_;
  Difficulty difficulty_;
  Traits traits_;
  std::set<std::string> knownRoutes_;
  std::set<std::string> appliedEvents_;
  double nowSeconds_ = 0;
  std::mt19937_64 random_;
};

} // namespace thug::companion
