#pragma once

#include "../HunterCore/HunterCore.h"

#include <map>
#include <string>
#include <vector>

namespace thug::save {

struct WorldObjectState {
  std::string stateId; // Authored by the actor: e.g. closed, open, collected.
  hunter::Vec3 positionMeters;
  hunter::Vec3 rotationDegrees;
  bool present = true;
};

struct WorldObjectDefinition {
  std::string id;        // Stable across editor sessions and save files.
  std::string archetype; // Authored actor type; not an asset path.
  WorldObjectState initial;
};

struct WorldSnapshot {
  std::string levelId;
  double nowSeconds = 0;
  std::vector<WorldObjectDefinition> catalog;
  std::map<std::string, WorldObjectState> objects;
};

// Stores only an authored level's persistent actors. Unreal decides which
// actors to include and applies the state after loading the level.
class WorldState {
public:
  WorldState(std::string levelId,
             const std::vector<WorldObjectDefinition>& definitions);
  bool SetObject(const std::string& id, WorldObjectState state,
                 double atSeconds);
  bool AdvanceTo(double atSeconds);
  double NowSeconds() const { return nowSeconds_; }
  const WorldObjectState* Find(const std::string& id) const;
  WorldSnapshot CaptureSnapshot() const;
  bool RestoreSnapshot(const WorldSnapshot& snapshot);

private:
  std::string levelId_;
  std::vector<WorldObjectDefinition> catalog_;
  std::map<std::string, WorldObjectState> objects_;
  double nowSeconds_ = 0;
};

} // namespace thug::save
