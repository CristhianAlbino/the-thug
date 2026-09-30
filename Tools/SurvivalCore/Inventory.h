#pragma once

#include <map>
#include <string>
#include <vector>

namespace thug::survival {

enum class ItemCategory { Weapon, Ammunition, Explosive, Medical, Utility,
                          Information };
enum class RemovalReason { Consume, Transfer, Drop };

// Authored item data; capacity and costs are not fixed in the GDD.
struct ItemDefinition {
  std::string id;
  ItemCategory category;
  int slotCost = 1;
  int maxQuantity = 1;
  bool droppable = true;
};

class Inventory {
public:
  Inventory(int capacitySlots, const std::vector<ItemDefinition>& definitions);

  bool CanAdd(const std::string& itemId, int quantity) const;
  bool Add(const std::string& itemId, int quantity);
  bool Remove(const std::string& itemId, int quantity, RemovalReason reason);
  // Checks both inventories first; either all units move or none do.
  bool TransferTo(Inventory& other, const std::string& itemId, int quantity);

  int Quantity(const std::string& itemId) const;
  bool IsCategory(const std::string& itemId, ItemCategory category) const;
  int CapacitySlots() const { return capacitySlots_; }
  long long UsedSlots() const;
  const std::map<std::string, int>& Contents() const { return quantities_; }

private:
  int capacitySlots_;
  std::map<std::string, ItemDefinition> definitions_;
  std::map<std::string, int> quantities_;
};

} // namespace thug::survival
