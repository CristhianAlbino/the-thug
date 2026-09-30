#include "Inventory.h"

#include <stdexcept>
#include <utility>

namespace thug::survival {

Inventory::Inventory(int capacitySlots,
                     const std::vector<ItemDefinition>& definitions)
    : capacitySlots_(capacitySlots) {
  if (capacitySlots < 0) {
    throw std::invalid_argument("Inventory capacity cannot be negative");
  }
  for (const auto& item : definitions) {
    if (item.id.empty() || item.slotCost <= 0 || item.maxQuantity <= 0 ||
        !definitions_.emplace(item.id, item).second) {
      throw std::invalid_argument("Invalid or duplicate item definition");
    }
  }
}

int Inventory::Quantity(const std::string& itemId) const {
  const auto it = quantities_.find(itemId);
  return it == quantities_.end() ? 0 : it->second;
}

bool Inventory::IsCategory(const std::string& itemId,
                           ItemCategory category) const {
  const auto it = definitions_.find(itemId);
  return it != definitions_.end() && it->second.category == category;
}

long long Inventory::UsedSlots() const {
  long long total = 0;
  for (const auto& [id, count] : quantities_) {
    total += static_cast<long long>(count) * definitions_.at(id).slotCost;
  }
  return total;
}

bool Inventory::CanAdd(const std::string& itemId, int quantity) const {
  const auto item = definitions_.find(itemId);
  if (item == definitions_.end() || quantity <= 0) return false;
  const auto newQuantity = static_cast<long long>(Quantity(itemId)) + quantity;
  return newQuantity <= item->second.maxQuantity &&
         UsedSlots() + static_cast<long long>(quantity) *
                           item->second.slotCost <= capacitySlots_;
}

bool Inventory::Add(const std::string& itemId, int quantity) {
  if (!CanAdd(itemId, quantity)) return false;
  quantities_[itemId] = Quantity(itemId) + quantity;
  return true;
}

bool Inventory::Remove(const std::string& itemId, int quantity,
                       RemovalReason reason) {
  const auto definition = definitions_.find(itemId);
  if (definition == definitions_.end() || quantity <= 0 ||
      Quantity(itemId) < quantity ||
      (reason != RemovalReason::Consume &&
       reason != RemovalReason::Transfer && reason != RemovalReason::Drop) ||
      (reason == RemovalReason::Drop && !definition->second.droppable)) {
    return false;
  }
  auto& stored = quantities_.at(itemId);
  stored -= quantity;
  if (stored == 0) quantities_.erase(itemId);
  return true;
}

bool Inventory::TransferTo(Inventory& other, const std::string& itemId,
                           int quantity) {
  if (&other == this || quantity <= 0 || Quantity(itemId) < quantity) {
    return false;
  }
  const auto source = definitions_.find(itemId);
  const auto target = other.definitions_.find(itemId);
  if (source == definitions_.end() || target == other.definitions_.end() ||
      source->second.category != target->second.category ||
      source->second.slotCost != target->second.slotCost ||
      source->second.maxQuantity != target->second.maxQuantity ||
      source->second.droppable != target->second.droppable ||
      !other.CanAdd(itemId, quantity)) {
    return false;
  }
  // All preconditions were checked; both operations are expected to succeed.
  other.Add(itemId, quantity);
  Remove(itemId, quantity, RemovalReason::Transfer);
  return true;
}

InventorySnapshot Inventory::CaptureSnapshot() const {
  std::vector<ItemDefinition> definitions;
  definitions.reserve(definitions_.size());
  for (const auto& [id, definition] : definitions_) definitions.push_back(definition);
  return {capacitySlots_, std::move(definitions), quantities_};
}

bool Inventory::RestoreSnapshot(const InventorySnapshot& snapshot) {
  if (snapshot.capacitySlots != capacitySlots_ ||
      snapshot.definitions.size() != definitions_.size()) return false;
  auto it = definitions_.begin();
  for (const auto& saved : snapshot.definitions) {
    if (saved.id != it->first ||
        saved.category != it->second.category ||
        saved.slotCost != it->second.slotCost ||
        saved.maxQuantity != it->second.maxQuantity ||
        saved.droppable != it->second.droppable) return false;
    ++it;
  }

  long long usedSlots = 0;
  for (const auto& [id, count] : snapshot.contents) {
    const auto definition = definitions_.find(id);
    if (definition == definitions_.end() || count <= 0 ||
        count > definition->second.maxQuantity) return false;
    usedSlots += static_cast<long long>(count) * definition->second.slotCost;
    if (usedSlots > capacitySlots_) return false;
  }
  auto quantities = snapshot.contents;
  quantities_.swap(quantities);
  return true;
}

} // namespace thug::survival
