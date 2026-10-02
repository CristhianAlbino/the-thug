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

// Only explicitly authored optional destructibles can be changed by combat.
// Doors, circuits, pickups and essential route objects retain their own contracts.
struct DestructibleDefinition {
  std::string id, node, before, after;
  bool gunshot = false, explosion = false;
};

struct ChargeSiteDefinition { std::string id, node; };
struct SpecialSupplyDefinition { std::string id, node, item; int quantity = 1; };
struct FinaleDefinition {
  std::string confrontationNode, confrontationExit, detonatorNode, collapseShelter;
  double confrontationSeconds = 3, opportunitySeconds = 8, plantSeconds = 4, collapseSeconds = 12;
  int minimumCompanions = 2;
  double minimumTrust = 60;
  std::vector<ChargeSiteDefinition> sites;
  std::vector<SpecialSupplyDefinition> supplies;
};
struct AppearanceDefinition {
  std::string id, node, block;
  unsigned modes = 15;
  int minimumAct = 3;
  unsigned tickets = 1000; // Out of 1000; Flash is an optional rare reference.
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
  std::vector<DestructibleDefinition> destructibles;
  std::optional<FinaleDefinition> finale;
  std::vector<AppearanceDefinition> appearances;
};

} // namespace thug::campaign
