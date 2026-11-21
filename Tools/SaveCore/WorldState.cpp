#include "WorldState.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace thug::save {

void swap(WorldState& a, WorldState& b) noexcept {
  static_assert(noexcept(a.levelId_.swap(b.levelId_)));
  static_assert(noexcept(a.catalog_.swap(b.catalog_)));
  static_assert(noexcept(a.objects_.swap(b.objects_)));
  a.levelId_.swap(b.levelId_);
  a.catalog_.swap(b.catalog_);
  a.objects_.swap(b.objects_);
  using std::swap;
  swap(a.nowSeconds_, b.nowSeconds_);
}
namespace {

bool Finite(const hunter::Vec3& value) {
  return std::isfinite(value.x) && std::isfinite(value.y) &&
         std::isfinite(value.z);
}

bool Valid(const WorldObjectState& state) {
  return !state.stateId.empty() && Finite(state.positionMeters) &&
         Finite(state.rotationDegrees);
}

bool Same(const WorldObjectState& a, const WorldObjectState& b) {
  return a.stateId == b.stateId &&
         a.positionMeters.x == b.positionMeters.x &&
         a.positionMeters.y == b.positionMeters.y &&
         a.positionMeters.z == b.positionMeters.z &&
         a.rotationDegrees.x == b.rotationDegrees.x &&
         a.rotationDegrees.y == b.rotationDegrees.y &&
         a.rotationDegrees.z == b.rotationDegrees.z &&
         a.present == b.present;
}

} // namespace

WorldState::WorldState(
    std::string levelId, const std::vector<WorldObjectDefinition>& definitions)
    : levelId_(std::move(levelId)) {
  if (levelId_.empty()) throw std::invalid_argument("Empty level ID");
  for (const auto& definition : definitions) {
    if (definition.id.empty() || definition.archetype.empty() ||
        !Valid(definition.initial) ||
        !objects_.emplace(definition.id, definition.initial).second)
      throw std::invalid_argument("Invalid or duplicate world object");
  }
  catalog_ = definitions;
  std::sort(catalog_.begin(), catalog_.end(),
            [](const auto& a, const auto& b) { return a.id < b.id; });
}

bool WorldState::SetObject(const std::string& id, WorldObjectState state,
                           double atSeconds) {
  auto it = objects_.find(id);
  if (it == objects_.end() || !Valid(state) ||
      !std::isfinite(atSeconds) || atSeconds < nowSeconds_) return false;
  it->second = std::move(state);
  nowSeconds_ = atSeconds;
  return true;
}

bool WorldState::AdvanceTo(double atSeconds) {
  if (!std::isfinite(atSeconds) || atSeconds < nowSeconds_) return false;
  nowSeconds_ = atSeconds;
  return true;
}

const WorldObjectState* WorldState::Find(const std::string& id) const {
  const auto it = objects_.find(id);
  return it == objects_.end() ? nullptr : &it->second;
}

WorldSnapshot WorldState::CaptureSnapshot() const {
  return {levelId_, nowSeconds_, catalog_, objects_};
}

bool WorldState::RestoreSnapshot(const WorldSnapshot& s) {
  if (s.levelId != levelId_ || !std::isfinite(s.nowSeconds) ||
      s.nowSeconds < 0 || s.catalog.size() != catalog_.size() ||
      s.objects.size() != objects_.size()) return false;
  for (std::size_t i = 0; i < catalog_.size(); ++i) {
    const auto& expected = catalog_[i];
    const auto& saved = s.catalog[i];
    if (saved.id != expected.id || saved.archetype != expected.archetype ||
        !Same(saved.initial, expected.initial)) return false;
  }
  for (const auto& [id, state] : s.objects)
    if (!objects_.count(id) || !Valid(state)) return false;
  auto objects = s.objects;
  objects_.swap(objects);
  nowSeconds_ = s.nowSeconds;
  return true;
}

} // namespace thug::save
