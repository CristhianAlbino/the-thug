#pragma once

#include "../WorldCore/WorldCore.h"
#include "../SurvivalCore/Inventory.h"
#include "../HunterCore/RunRules.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace thug::campaign {

struct ObjectRequirement {
  std::string id;
  std::string state;
};

struct PickupDefinition {
  std::string id;
  std::string node;
  std::string item;
  int quantity = 1;
  std::optional<rules::Event> onCollect;
  std::vector<ObjectRequirement> requirements;
};

// A named, authored interaction, never an arbitrary UI request to grant items.
struct UseDefinition {
  std::string id;
  std::string node;
  std::string object;
  std::string before;
  std::string after;
  std::string item;
  int quantity = 0;
  bool consume = false;
  std::optional<rules::Event> runEvent;
  std::vector<ObjectRequirement> requirements;
};

struct UnlockDefinition {
  std::string id;
  std::string node;
  std::string door;
  std::string item;
  int quantity = 1;
  bool consume = false;
};

struct TriggerDefinition {
  std::string id;
  std::string node;
  rules::Event event;
  std::vector<ObjectRequirement> requirements;
};

struct WorldDefinition {
  std::string levelId;
  std::set<std::string> nodes;
  std::vector<save::WorldObjectDefinition> objects;
  std::vector<world::DoorDefinition> doors;
  std::vector<world::CameraDefinition> cameras;
  std::vector<world::HideDefinition> hides;
  std::map<std::string, std::string> circuitNodes;
  std::vector<PickupDefinition> pickups;
  std::vector<UseDefinition> uses;
  std::vector<UnlockDefinition> unlocks;
  std::vector<TriggerDefinition> triggers;
};

} // namespace thug::campaign
