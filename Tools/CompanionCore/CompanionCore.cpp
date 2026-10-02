#include "CompanionCore.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace thug::companion {

void swap(CompanionCore& a, CompanionCore& b) noexcept {
  static_assert(noexcept(a.knownRoutes_.swap(b.knownRoutes_)));
  static_assert(noexcept(a.appliedEvents_.swap(b.appliedEvents_)));
  static_assert(std::is_nothrow_swappable_v<Traits>);
  static_assert(std::is_nothrow_swappable_v<std::mt19937_64>);
  using std::swap;
  swap(a.character_, b.character_);
  swap(a.difficulty_, b.difficulty_);
  swap(a.traits_, b.traits_);
  a.knownRoutes_.swap(b.knownRoutes_);
  a.appliedEvents_.swap(b.appliedEvents_);
  swap(a.nowSeconds_, b.nowSeconds_);
  swap(a.random_, b.random_);
}
namespace {

bool InRange(double value) {
  return std::isfinite(value) && value >= 0 && value <= 100;
}

bool Finite(const Influence& value) {
  return std::isfinite(value.trust) && std::isfinite(value.fear) &&
         std::isfinite(value.stress) && std::isfinite(value.loyalty) &&
         std::isfinite(value.selfPreservation) &&
         std::isfinite(value.relationship);
}

std::optional<Influence> ExperienceInfluence(Experience experience) {
  Influence delta;
  switch (experience) {
  case Experience::JoeSharedAmmo:
    // GDD section 51: Trust +20, Loyalty +10, Resource +1.
    delta.trust = 20;
    delta.loyalty = 10;
    delta.ammunition = 1;
    return delta;
  case Experience::JoeAbandonedCompanion:
    // GDD section 51: Trust -40, Fear +20, SelfPreservation +30.
    delta.trust = -40;
    delta.fear = 20;
    delta.selfPreservation = 30;
    return delta;
  }
  return std::nullopt;
}

} // namespace

CompanionCore::CompanionCore(Character character, Difficulty difficulty,
                             Traits initial, std::uint64_t seed)
    : character_(character), difficulty_(difficulty), traits_(initial),
      random_(seed) {
  if (!ValidTraits(initial)) {
    throw std::invalid_argument("Invalid initial companion traits");
  }
}

bool CompanionCore::ValidTraits(const Traits& traits) {
  return InRange(traits.trust) && InRange(traits.fear) &&
         InRange(traits.stress) && InRange(traits.loyalty) &&
         InRange(traits.selfPreservation) && InRange(traits.relationship) &&
         traits.ammunition >= 0;
}

double CompanionCore::Clamp(double value) {
  return std::clamp(value, 0.0, 100.0);
}

void CompanionCore::AdvanceTo(double atSeconds) {
  if (!std::isfinite(atSeconds) || atSeconds < nowSeconds_) {
    throw std::invalid_argument("Companion game time must be monotonic");
  }
  nowSeconds_ = atSeconds;
}

bool CompanionCore::ApplyInfluence(const Influence& delta,
                                   const std::string& eventId,
                                   double atSeconds) {
  if (eventId.empty() || appliedEvents_.count(eventId) || !Finite(delta)) {
    return false;
  }
  const auto newAmmo = static_cast<long long>(traits_.ammunition) +
                       static_cast<long long>(delta.ammunition);
  if (newAmmo < 0 || newAmmo > std::numeric_limits<int>::max()) {
    return false;
  }
  AdvanceTo(atSeconds);
  traits_.trust = Clamp(traits_.trust + delta.trust);
  traits_.fear = Clamp(traits_.fear + delta.fear);
  traits_.stress = Clamp(traits_.stress + delta.stress);
  traits_.loyalty = Clamp(traits_.loyalty + delta.loyalty);
  traits_.selfPreservation =
      Clamp(traits_.selfPreservation + delta.selfPreservation);
  traits_.relationship = Clamp(traits_.relationship + delta.relationship);
  traits_.ammunition = static_cast<int>(newAmmo);
  appliedEvents_.insert(eventId);
  return true;
}

bool CompanionCore::CanApplyExperience(Experience experience,
                                       const std::string& eventId,
                                       double atSeconds) const {
  const auto delta = ExperienceInfluence(experience);
  if (!delta || eventId.empty() || appliedEvents_.count(eventId) ||
      !std::isfinite(atSeconds) || atSeconds < nowSeconds_) return false;
  const auto newAmmo = static_cast<long long>(traits_.ammunition) +
                       static_cast<long long>(delta->ammunition);
  return newAmmo >= 0 && newAmmo <= std::numeric_limits<int>::max();
}

bool CompanionCore::ApplyExperience(Experience experience,
                                    const std::string& eventId,
                                    double atSeconds) {
  const auto delta = ExperienceInfluence(experience);
  return delta && ApplyInfluence(*delta, eventId, atSeconds);
}

bool CompanionCore::LearnRoute(const std::string& routeId,
                               double atSeconds) {
  if (routeId.empty() || knownRoutes_.count(routeId)) return false;
  AdvanceTo(atSeconds);
  knownRoutes_.insert(routeId);
  return true;
}

void CompanionCore::SetInjured(bool injured, double atSeconds) {
  AdvanceTo(atSeconds);
  traits_.injured = injured;
}

std::vector<Option> CompanionCore::AvailableOptions(
    const Situation& situation) const {
  std::vector<Option> options;
  const double fearPressure = (traits_.fear + traits_.stress) / 200.0;
  options.push_back({Goal::StayHidden,
                     1.0 + 2.0 * fearPressure + (traits_.injured ? 1.0 : 0.0),
                     std::nullopt});

  if (situation.resourcesKnownNearby && !traits_.injured) {
    options.push_back({Goal::SearchResources,
                       1.0 + (1.0 - fearPressure) +
                           (traits_.ammunition == 0 ? 0.5 : 0.0),
                       std::nullopt});
  }
  if (situation.joePerceivedNearby && situation.joeNeedsHelp &&
      situation.canReachJoe && !traits_.injured) {
    const double bond = (traits_.trust + traits_.loyalty +
                         traits_.relationship) / 300.0;
    options.push_back({Goal::HelpJoe,
                       0.2 + 4.0 * bond * (1.0 - fearPressure * 0.7),
                       std::nullopt});
  }
  if (situation.joePerceivedNearby && !knownRoutes_.empty()) {
    options.push_back({Goal::ShareRoute,
                       0.2 + traits_.trust / 40.0,
                       *knownRoutes_.begin()});
  }
  const bool knowsReachableExit =
      situation.accessibleEscapeRoute &&
      knownRoutes_.count(*situation.accessibleEscapeRoute);
  if (knowsReachableExit && difficulty_ != Difficulty::Easy) {
    const double impulse =
        (traits_.selfPreservation + traits_.fear - traits_.loyalty) / 100.0;
    const double caution = difficulty_ == Difficulty::Normal ? 0.2 : 1.0;
    options.push_back({Goal::EscapeAlone,
                       caution * std::max(0.05, impulse),
                       situation.accessibleEscapeRoute});
  }
  if ((difficulty_ == Difficulty::Hard ||
       difficulty_ == Difficulty::Punitive) &&
      situation.hunterPerceivedNearby && situation.joePerceivedNearby &&
      traits_.trust < 40 && traits_.loyalty < 50 &&
      traits_.selfPreservation > 60) {
    options.push_back({Goal::UseJoeAsDistraction,
                       0.15 + (traits_.selfPreservation - traits_.trust) / 100.0,
                       std::nullopt});
  }
  return options;
}

Option CompanionCore::Choose(const Situation& situation) {
  return ChooseAmong(AvailableOptions(situation));
}

Option CompanionCore::ChooseAmong(const std::vector<Option>& options) {
  if (options.empty()) throw std::invalid_argument("Empty companion options");
  double total = 0;
  for (const auto& option : options) {
    const bool routeGoal = option.goal == Goal::ShareRoute || option.goal == Goal::EscapeAlone;
    if (option.goal < Goal::StayHidden || option.goal > Goal::UseJoeAsDistraction ||
        !std::isfinite(option.weight) || option.weight <= 0 ||
        routeGoal != bool(option.routeId) || (option.routeId && !knownRoutes_.count(*option.routeId)) ||
        (option.goal == Goal::EscapeAlone && difficulty_ == Difficulty::Easy) ||
        (option.goal == Goal::UseJoeAsDistraction && difficulty_ != Difficulty::Hard && difficulty_ != Difficulty::Punitive))
      throw std::invalid_argument("Invalid executable companion option");
    total += option.weight;
  }
  if (!std::isfinite(total)) throw std::invalid_argument("Invalid companion weights");
  std::vector<double> weights;
  weights.reserve(options.size());
  for (const auto& option : options) weights.push_back(option.weight);
  std::discrete_distribution<std::size_t> pick(weights.begin(), weights.end());
  return options[pick(random_)];
}

CompanionSnapshot CompanionCore::CaptureSnapshot() const {
  std::ostringstream randomStream;
  randomStream.imbue(std::locale::classic());
  randomStream << random_;
  return {character_, difficulty_, traits_, nowSeconds_, knownRoutes_,
          appliedEvents_, randomStream.str()};
}

bool CompanionCore::RestoreSnapshot(const CompanionSnapshot& snapshot) {
  if (snapshot.character != character_ || snapshot.difficulty != difficulty_ ||
      !ValidTraits(snapshot.traits) || !std::isfinite(snapshot.nowSeconds) ||
      snapshot.nowSeconds < 0 || snapshot.randomState.empty()) return false;
  for (const auto& id : snapshot.knownRoutes) if (id.empty()) return false;
  for (const auto& id : snapshot.appliedEvents) if (id.empty()) return false;

  std::istringstream randomStream(snapshot.randomState);
  randomStream.imbue(std::locale::classic());
  std::mt19937_64 restoredRandom;
  if (!(randomStream >> restoredRandom)) return false;
  randomStream >> std::ws;
  if (!randomStream.eof()) return false;

  // Allocate all replacement state before touching the live companion.
  auto routes = snapshot.knownRoutes;
  auto events = snapshot.appliedEvents;
  traits_ = snapshot.traits;
  nowSeconds_ = snapshot.nowSeconds;
  knownRoutes_.swap(routes);
  appliedEvents_.swap(events);
  random_ = restoredRandom;
  return true;
}

} // namespace thug::companion
