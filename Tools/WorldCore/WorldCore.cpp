#include "WorldCore.h"

#include <stdexcept>
#include <utility>

namespace thug::world {
namespace {

const save::WorldObjectState& Object(const save::WorldState& state,
                                     const std::string& id) {
  const auto* result = state.Find(id);
  if (!result) throw std::invalid_argument("Missing authored world object: " + id);
  return *result;
}

bool Allowed(const std::string& state, std::initializer_list<const char*> values) {
  for (const char* value : values) if (state == value) return true;
  return false;
}

} // namespace

WorldCore::WorldCore(save::WorldState& state, std::vector<DoorDefinition> doors,
                     std::vector<CameraDefinition> cameras,
                     std::vector<HideDefinition> hides) : state_(state) {
  const auto catalog = state.CaptureSnapshot().catalog;
  std::map<std::string, std::string> types;
  for (const auto& object : catalog) types.emplace(object.id, object.archetype);
  const auto expect = [&](const std::string& id, const char* type) {
    if (id.empty() || types.find(id) == types.end() || types.at(id) != type)
      throw std::invalid_argument("Wrong authored object type: " + id);
  };
  for (auto& door : doors) {
    expect(door.id, "door");
    const auto id = door.id;
    const auto& current = Object(state_, door.id).stateId;
    if (!door.circuitId.empty()) {
      expect(door.circuitId, "circuit");
      circuits_.insert(door.circuitId);
    }
    if (door.fromNode.empty() || door.toNode.empty() ||
        door.fromNode == door.toNode ||
        !Allowed(current, {"open", "forced_open", "closed", "locked", "jammed"}) ||
        ((door.power == DoorPower::Manual) != door.circuitId.empty()) ||
        (!door.circuitId.empty() &&
         !Allowed(Object(state_, door.circuitId).stateId, {"on", "off"})) ||
        !doors_.emplace(id, std::move(door)).second)
      throw std::invalid_argument("Invalid door definition");
  }
  for (auto& camera : cameras) {
    expect(camera.id, "camera");
    expect(camera.circuitId, "circuit");
    circuits_.insert(camera.circuitId);
    const auto id = camera.id;
    if (camera.atNode.empty() || camera.viewNode.empty() ||
        !Allowed(Object(state_, camera.circuitId).stateId, {"on", "off"}) ||
        !Allowed(Object(state_, camera.id).stateId,
                 {"online", "interference", "broken", "looped"}) ||
        !cameras_.emplace(id, std::move(camera)).second)
      throw std::invalid_argument("Invalid camera definition");
  }
  for (auto& hide : hides) {
    expect(hide.id, "hide");
    const auto id = hide.id;
    if (hide.atNode.empty() ||
        !Allowed(Object(state_, hide.id).stateId, {"empty", "occupied"}) ||
        !hides_.emplace(id, std::move(hide)).second)
      throw std::invalid_argument("Invalid hiding place definition");
  }
}

bool WorldCore::Update(const std::string& id, const std::string& stateId,
                       double atSeconds) {
  auto next = Object(state_, id);
  if (!next.present || next.stateId == stateId) return false;
  next.stateId = stateId;
  return state_.SetObject(id, std::move(next), atSeconds);
}

bool WorldCore::Powered(const std::string& circuitId) const {
  if (circuits_.count(circuitId) == 0) throw std::out_of_range("Unknown circuit");
  const auto& circuit = Object(state_, circuitId);
  return circuit.present && circuit.stateId == "on";
}

bool WorldCore::CanTraverse(const std::string& doorId) const {
  if (doors_.count(doorId) == 0) throw std::out_of_range("Unknown door");
  const auto& current = Object(state_, doorId);
  // Power governs actuation. An open physical passage cannot become an
  // invisible wall: automatic closure must be a separate, observed action.
  return current.present &&
         (current.stateId == "forced_open" || current.stateId == "open");
}

std::optional<WorldSignal> WorldCore::OpenDoor(const std::string& doorId,
                                               double atSeconds, bool force) {
  const auto& door = doors_.at(doorId);
  const auto& current = Object(state_, doorId);
  if (!current.present || current.stateId == "open" || current.stateId == "forced_open" ||
      current.stateId == "jammed") return std::nullopt;
  if (current.stateId == "locked" && !force) return std::nullopt;
  if (door.power == DoorPower::RequiresPower && !Powered(door.circuitId) &&
      !force) return std::nullopt;
  const auto position = current.positionMeters;
  if (!Update(doorId, force ? "forced_open" : "open", atSeconds))
    return std::nullopt;
  return WorldSignal{doorId,
                     force ? perception::SoundKind::BrokenObject
                           : perception::SoundKind::Door,
                     position, atSeconds, force ? 1.0 : 0.35};
}

bool WorldCore::CloseDoor(const std::string& doorId, double atSeconds) {
  if (doors_.count(doorId) == 0 ||
      !Allowed(Object(state_, doorId).stateId, {"open", "forced_open"}))
    return false;
  return Update(doorId, "closed", atSeconds);
}

bool WorldCore::UnlockDoor(const std::string& doorId, double atSeconds) {
  if (doors_.count(doorId) == 0 || Object(state_, doorId).stateId != "locked")
    return false;
  return Update(doorId, "closed", atSeconds);
}

bool WorldCore::JamDoor(const std::string& doorId, double atSeconds) {
  if (doors_.count(doorId) == 0 || Object(state_, doorId).stateId == "jammed")
    return false;
  return Update(doorId, "jammed", atSeconds);
}

std::optional<WorldSignal> WorldCore::SetCircuit(
    const std::string& circuitId, bool powered, double atSeconds) {
  if (circuits_.count(circuitId) == 0) throw std::out_of_range("Unknown circuit");
  const auto& current = Object(state_, circuitId);
  if (current.stateId != "on" && current.stateId != "off")
    throw std::invalid_argument("Not a circuit");
  const auto position = current.positionMeters;
  if (!Update(circuitId, powered ? "on" : "off", atSeconds)) return std::nullopt;
  return WorldSignal{circuitId, perception::SoundKind::Distraction, position,
                     atSeconds, 0.5}; // Provisional relay cue; UE authors real audio.
}

bool WorldCore::SetCameraState(const std::string& cameraId,
                               const std::string& stateId, double atSeconds) {
  if (cameras_.count(cameraId) == 0 ||
      !Allowed(stateId, {"online", "interference", "broken", "looped"}))
    return false;
  return Update(cameraId, stateId, atSeconds);
}

std::optional<Feed> WorldCore::ReadCamera(const std::string& cameraId) const {
  const auto& camera = cameras_.at(cameraId);
  if (!Object(state_, cameraId).present || !Powered(camera.circuitId))
    return std::nullopt;
  const auto& state = Object(state_, cameraId).stateId;
  if (state == "online") return Feed{camera.viewNode, true};
  if (state == "looped") return Feed{camera.viewNode, false};
  return std::nullopt;
}

bool WorldCore::EnterHide(const std::string& hideId, double atSeconds) {
  if (hides_.count(hideId) == 0 || Occupied(hideId)) return false;
  // Knowledge of this act belongs to sensors, not to WorldCore or Batman.
  return Update(hideId, "occupied", atSeconds);
}

bool WorldCore::LeaveHide(const std::string& hideId, double atSeconds) {
  if (hides_.count(hideId) == 0 || !Occupied(hideId)) return false;
  return Update(hideId, "empty", atSeconds);
}

bool WorldCore::Occupied(const std::string& hideId) const {
  if (hides_.count(hideId) == 0) throw std::out_of_range("Unknown hide place");
  const auto& hide = Object(state_, hideId);
  return hide.present && hide.stateId == "occupied";
}

} // namespace thug::world
