#pragma once

#include "../SaveCore/WorldState.h"
#include "../PerceptionCore/PerceptionCore.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace thug::world {

enum class DoorPower { Manual, RequiresPower, ReleasesWithoutPower };

struct DoorDefinition {
  std::string id;
  std::string fromNode;
  std::string toNode;
  std::string circuitId; // Empty for a manual door.
  DoorPower power = DoorPower::Manual;
};

struct CameraDefinition {
  std::string id;
  std::string atNode;
  std::string viewNode;
  std::string circuitId;
};

struct HideDefinition {
  std::string id;
  std::string atNode;
};

// A physical source for the perception adapter. The caller supplies occlusion,
// masking and a real listener; an event is not automatically heard by Batman.
struct WorldSignal {
  std::string objectId;
  perception::SoundKind sound;
  hunter::Vec3 positionMeters;
  double atSeconds;
  double energy;
};

struct Feed {
  std::string viewNode;
  bool live; // A looped frame is not current knowledge.
};

class WorldCore {
public:
  WorldCore(save::WorldState& state, std::vector<DoorDefinition> doors,
            std::vector<CameraDefinition> cameras,
            std::vector<HideDefinition> hides);

  bool CanTraverse(const std::string& doorId) const;
  std::optional<WorldSignal> OpenDoor(const std::string& doorId,
                                      double atSeconds, bool force = false);
  bool CloseDoor(const std::string& doorId, double atSeconds);
  // The coordinator must validate the physical key/tool or objective first.
  bool UnlockDoor(const std::string& doorId, double atSeconds);
  bool JamDoor(const std::string& doorId, double atSeconds);

  std::optional<WorldSignal> SetCircuit(const std::string& circuitId,
                                        bool powered, double atSeconds);
  bool Powered(const std::string& circuitId) const;
  bool SetCameraState(const std::string& cameraId,
                      const std::string& stateId, double atSeconds);
  std::optional<Feed> ReadCamera(const std::string& cameraId) const;

  bool EnterHide(const std::string& hideId, double atSeconds);
  bool LeaveHide(const std::string& hideId, double atSeconds);
  bool Occupied(const std::string& hideId) const;

private:
  bool Update(const std::string& id, const std::string& stateId,
              double atSeconds);
  save::WorldState& state_;
  std::map<std::string, DoorDefinition> doors_;
  std::map<std::string, CameraDefinition> cameras_;
  std::map<std::string, HideDefinition> hides_;
  std::set<std::string> circuits_;
};

} // namespace thug::world
