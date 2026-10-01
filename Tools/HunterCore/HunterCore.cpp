#include "HunterCore.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <type_traits>
#include <utility>

namespace thug::hunter {

void swap(HunterCore& a, HunterCore& b) noexcept {
  static_assert(std::is_nothrow_swappable_v<Tuning>);
  static_assert(std::is_nothrow_swappable_v<std::mt19937_64>);
  static_assert(noexcept(a.lastEstimate_.swap(b.lastEstimate_)));
  static_assert(noexcept(a.lastHeardType_.swap(b.lastHeardType_)));
  static_assert(noexcept(a.observedRoutes_.swap(b.observedRoutes_)));
  static_assert(noexcept(a.confirmedHideSpots_.swap(b.confirmedHideSpots_)));
  using std::swap;
  swap(a.tuning_, b.tuning_);
  swap(a.random_, b.random_);
  swap(a.state_, b.state_);
  swap(a.nowSeconds_, b.nowSeconds_);
  swap(a.enteredStateAt_, b.enteredStateAt_);
  a.lastEstimate_.swap(b.lastEstimate_);
  a.lastHeardType_.swap(b.lastHeardType_);
  a.observedRoutes_.swap(b.observedRoutes_);
  a.confirmedHideSpots_.swap(b.confirmedHideSpots_);
}
namespace {

bool Finite(double value) { return std::isfinite(value); }

bool ValidPosition(const Vec3& value) {
  return Finite(value.x) && Finite(value.y) && Finite(value.z);
}

bool SameTuning(const Tuning& a, const Tuning& b) {
  return a.minimumSoundRadiusMeters == b.minimumSoundRadiusMeters &&
         a.soundThreshold == b.soundThreshold &&
         a.searchSeconds == b.searchSeconds &&
         a.investigateSeconds == b.investigateSeconds &&
         a.memoryHalfLifeSeconds == b.memoryHalfLifeSeconds &&
         a.locationLifetimeSeconds == b.locationLifetimeSeconds &&
         a.minimumRouteWeight == b.minimumRouteWeight &&
         a.routeEvidenceWeight == b.routeEvidenceWeight &&
         a.hideSuspicionCap == b.hideSuspicionCap;
}

} // namespace

Tuning TuningFor(Difficulty difficulty) {
  Tuning tuning;
  switch (difficulty) {
  case Difficulty::Easy:
    tuning.soundThreshold = 0.30;
    tuning.searchSeconds = 18;
    tuning.investigateSeconds = 10;
    tuning.memoryHalfLifeSeconds = 90;
    tuning.locationLifetimeSeconds = 45;
    tuning.routeEvidenceWeight = 0.5;
    break;
  case Difficulty::Normal:
    tuning.soundThreshold = 0.14;
    tuning.searchSeconds = 28;
    tuning.memoryHalfLifeSeconds = 180;
    tuning.locationLifetimeSeconds = 75;
    tuning.routeEvidenceWeight = 1;
    break;
  case Difficulty::Hard:
    break;
  case Difficulty::Punitive:
    tuning.soundThreshold = 0.05;
    tuning.searchSeconds = 65;
    tuning.investigateSeconds = 22;
    tuning.memoryHalfLifeSeconds = 480;
    tuning.locationLifetimeSeconds = 180;
    tuning.routeEvidenceWeight = 3;
    break;
  default:
    throw std::invalid_argument("Unknown Hunter difficulty");
  }
  return tuning;
}

HunterCore::HunterCore(Difficulty difficulty, std::uint64_t seed)
    : HunterCore(TuningFor(difficulty), seed) {}

HunterCore::HunterCore(Tuning tuning, std::uint64_t seed)
    : tuning_(tuning), random_(seed) {
  if (!Finite(tuning_.minimumSoundRadiusMeters) ||
      tuning_.minimumSoundRadiusMeters <= 0 ||
      !Finite(tuning_.soundThreshold) || tuning_.soundThreshold < 0 ||
      tuning_.soundThreshold > 1 || !Finite(tuning_.searchSeconds) ||
      tuning_.searchSeconds <= 0 || !Finite(tuning_.investigateSeconds) ||
      tuning_.investigateSeconds <= 0 ||
      !Finite(tuning_.memoryHalfLifeSeconds) ||
      tuning_.memoryHalfLifeSeconds <= 0 ||
      !Finite(tuning_.locationLifetimeSeconds) ||
      tuning_.locationLifetimeSeconds <= 0 ||
      !Finite(tuning_.minimumRouteWeight) ||
      tuning_.minimumRouteWeight <= 0 ||
      !Finite(tuning_.routeEvidenceWeight) ||
      tuning_.routeEvidenceWeight < 0 ||
      !Finite(tuning_.hideSuspicionCap) ||
      tuning_.hideSuspicionCap <= 0) {
    throw std::invalid_argument("Invalid HunterCore tuning");
  }
}

bool HunterCore::Valid(const Stimulus& stimulus) const {
  if (!Finite(stimulus.atSeconds) || stimulus.atSeconds < nowSeconds_ ||
      !Finite(stimulus.strength) || stimulus.strength < 0 ||
      stimulus.strength > 1) {
    return false;
  }
  if (stimulus.estimate) {
    const auto& estimate = *stimulus.estimate;
    if (!ValidPosition(estimate.center) || !Finite(estimate.radiusMeters) ||
        estimate.radiusMeters < 0 ||
        !Finite(estimate.observedAtSeconds) ||
        std::abs(estimate.observedAtSeconds - stimulus.atSeconds) > 0.000001) {
      return false;
    }
  }
  switch (stimulus.kind) {
  case StimulusKind::Sound:
    return stimulus.estimate && stimulus.observedId.empty() &&
           stimulus.estimate->radiusMeters >= tuning_.minimumSoundRadiusMeters;
  case StimulusKind::VisualContact:
    return stimulus.estimate && stimulus.observedId.empty() &&
           stimulus.subject != Subject::Unknown && stimulus.strength > 0;
  case StimulusKind::VisualLost:
    return !stimulus.estimate && stimulus.observedId.empty() &&
           stimulus.subject == Subject::Player;
  case StimulusKind::EnvironmentalEvidence:
    return stimulus.estimate && stimulus.observedId.empty() &&
           stimulus.estimate->radiusMeters > 0;
  case StimulusKind::ObservedRoute:
  case StimulusKind::ConfirmedHideSpot:
    return !stimulus.estimate && stimulus.subject == Subject::Player &&
           !stimulus.observedId.empty() && stimulus.strength > 0;
  }
  return false;
}

void HunterCore::SetState(State state, double atSeconds) {
  state_ = state;
  enteredStateAt_ = atSeconds;
}

bool HunterCore::Observe(const Stimulus& stimulus) {
  if (!Valid(stimulus)) {
    return false;
  }
  AdvanceTo(stimulus.atSeconds);

  switch (stimulus.kind) {
  case StimulusKind::VisualContact:
    if (stimulus.subject == Subject::Player || state_ != State::Pursuing) {
      lastEstimate_ = stimulus.estimate;
      SetState(stimulus.subject == Subject::Player ? State::Pursuing
                                                   : State::Investigating,
               nowSeconds_);
    }
    break;
  case StimulusKind::VisualLost:
    if (state_ == State::Pursuing) {
      SetState(State::Searching, nowSeconds_);
    }
    break;
  case StimulusKind::Sound:
    lastHeardType_ = stimulus.perceivedType;
    if (stimulus.strength >= tuning_.soundThreshold &&
        state_ != State::Pursuing) {
      lastEstimate_ = stimulus.estimate;
      SetState(State::Investigating, nowSeconds_);
    }
    break;
  case StimulusKind::EnvironmentalEvidence:
    if (stimulus.strength >= tuning_.soundThreshold &&
        state_ != State::Pursuing) {
      lastEstimate_ = stimulus.estimate;
      SetState(State::Investigating, nowSeconds_);
    }
    break;
  case StimulusKind::ObservedRoute: {
    auto& entry = observedRoutes_[stimulus.observedId];
    entry.score = DecayedScore(entry) + stimulus.strength;
    entry.lastObservedAt = nowSeconds_;
    ++entry.observations;
    break;
  }
  case StimulusKind::ConfirmedHideSpot: {
    auto& entry = confirmedHideSpots_[stimulus.observedId];
    // 10, 30, 60, 90 for four promptly observed uses, as in the GDD.
    const double increment = entry.observations == 0 ? 10 :
                             entry.observations == 1 ? 20 : 30;
    entry.score = std::min(tuning_.hideSuspicionCap,
                           DecayedScore(entry) + increment * stimulus.strength);
    entry.lastObservedAt = nowSeconds_;
    ++entry.observations;
    break;
  }
  }
  return true;
}

void HunterCore::AdvanceTo(double nowSeconds) {
  if (!Finite(nowSeconds) || nowSeconds < nowSeconds_) {
    throw std::invalid_argument("Game time must be finite and monotonic");
  }
  nowSeconds_ = nowSeconds;
  if (state_ == State::Investigating &&
      nowSeconds_ - enteredStateAt_ >= tuning_.investigateSeconds) {
    SetState(State::Searching, enteredStateAt_ + tuning_.investigateSeconds);
  }
  if (state_ == State::Searching &&
      nowSeconds_ - enteredStateAt_ >= tuning_.searchSeconds) {
    SetState(State::Patrol, nowSeconds_);
    lastEstimate_.reset();
  }
}

Intent HunterCore::CurrentIntent() const {
  Intent intent;
  intent.state = state_;
  if (state_ != State::Patrol && lastEstimate_ &&
      nowSeconds_ - lastEstimate_->observedAtSeconds <=
          tuning_.locationLifetimeSeconds) {
    intent.destination = lastEstimate_;
  }
  return intent;
}

double HunterCore::DecayedScore(const Evidence& value) const {
  return value.score * std::exp2(-(nowSeconds_ - value.lastObservedAt) /
                                 tuning_.memoryHalfLifeSeconds);
}

double HunterCore::HideSpotSuspicion(const std::string& id) const {
  const auto it = confirmedHideSpots_.find(id);
  return it == confirmedHideSpots_.end() ? 0 : DecayedScore(it->second);
}

double HunterCore::RouteEvidence(const std::string& id) const {
  const auto it = observedRoutes_.find(id);
  return it == observedRoutes_.end() ? 0 : DecayedScore(it->second);
}

std::string HunterCore::ChoosePatrolRoute(
    const std::vector<std::string>& availableRoutes) {
  if (availableRoutes.empty()) {
    throw std::invalid_argument("At least one navigable route is required");
  }
  std::unordered_set<std::string> unique;
  for (const auto& id : availableRoutes) {
    if (id.empty() || !unique.insert(id).second) {
      throw std::invalid_argument("Route IDs must be non-empty and unique");
    }
  }
  // Exploration gives other valid routes a real chance even after repetition.
  if (availableRoutes.size() > 1 &&
      std::bernoulli_distribution(0.15)(random_)) {
    std::uniform_int_distribution<std::size_t> any(0, availableRoutes.size() - 1);
    return availableRoutes[any(random_)];
  }
  std::vector<double> weights;
  weights.reserve(availableRoutes.size());
  for (const auto& id : availableRoutes) {
    weights.push_back(tuning_.minimumRouteWeight +
                      tuning_.routeEvidenceWeight * RouteEvidence(id));
  }
  std::discrete_distribution<std::size_t> choice(weights.begin(), weights.end());
  return availableRoutes[choice(random_)];
}

HunterSnapshot HunterCore::CaptureSnapshot() const {
  HunterSnapshot snapshot;
  snapshot.tuning = tuning_;
  snapshot.state = state_;
  snapshot.nowSeconds = nowSeconds_;
  snapshot.enteredStateAt = enteredStateAt_;
  snapshot.lastEstimate = lastEstimate_;
  snapshot.lastHeardType = lastHeardType_;
  for (const auto& [id, e] : observedRoutes_)
    snapshot.observedRoutes.emplace(id, EvidenceSnapshot{e.score,
                                     e.lastObservedAt, e.observations});
  for (const auto& [id, e] : confirmedHideSpots_)
    snapshot.confirmedHideSpots.emplace(id, EvidenceSnapshot{e.score,
                                       e.lastObservedAt, e.observations});
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << random_;
  snapshot.randomState = out.str();
  return snapshot;
}

bool HunterCore::RestoreSnapshot(const HunterSnapshot& snapshot) {
  if (!SameTuning(snapshot.tuning, tuning_) ||
      snapshot.state < State::Patrol || snapshot.state > State::Searching ||
      !Finite(snapshot.nowSeconds) || snapshot.nowSeconds < 0 ||
      !Finite(snapshot.enteredStateAt) || snapshot.enteredStateAt < 0 ||
      snapshot.enteredStateAt > snapshot.nowSeconds ||
      snapshot.randomState.empty()) return false;
  if (snapshot.lastEstimate) {
    const auto& value = *snapshot.lastEstimate;
    if (!ValidPosition(value.center) || !Finite(value.radiusMeters) ||
        value.radiusMeters < 0 || !Finite(value.observedAtSeconds) ||
        value.observedAtSeconds < 0 ||
        value.observedAtSeconds > snapshot.nowSeconds) return false;
  }
  auto validEvidence = [&](const auto& entries, bool hideSpots) {
    for (const auto& [id, e] : entries) {
      if (id.empty() || !Finite(e.score) || e.score < 0 ||
          (hideSpots && e.score > tuning_.hideSuspicionCap) ||
          !Finite(e.lastObservedAt) || e.lastObservedAt < 0 ||
          e.lastObservedAt > snapshot.nowSeconds || e.observations <= 0)
        return false;
    }
    return true;
  };
  if (!validEvidence(snapshot.observedRoutes, false) ||
      !validEvidence(snapshot.confirmedHideSpots, true)) return false;
  std::istringstream in(snapshot.randomState);
  in.imbue(std::locale::classic());
  std::mt19937_64 restoredRandom;
  if (!(in >> restoredRandom)) return false;
  in >> std::ws;
  if (!in.eof()) return false;

  std::map<std::string, Evidence> routes, spots;
  for (const auto& [id, e] : snapshot.observedRoutes)
    routes.emplace(id, Evidence{e.score, e.lastObservedAt, e.observations});
  for (const auto& [id, e] : snapshot.confirmedHideSpots)
    spots.emplace(id, Evidence{e.score, e.lastObservedAt, e.observations});
  auto estimate = snapshot.lastEstimate;
  auto heardType = snapshot.lastHeardType;
  observedRoutes_.swap(routes);
  confirmedHideSpots_.swap(spots);
  lastEstimate_.swap(estimate);
  lastHeardType_.swap(heardType);
  state_ = snapshot.state;
  nowSeconds_ = snapshot.nowSeconds;
  enteredStateAt_ = snapshot.enteredStateAt;
  random_ = restoredRandom;
  return true;
}

} // namespace thug::hunter
