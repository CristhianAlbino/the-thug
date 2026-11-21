#include "CampaignState.h"
#include "CompanionIO.h"
#include "../IntegrationCore/IntegrationCore.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <utility>

namespace thug::campaign {
namespace {
template <typename T>
const T* Find(const std::vector<T>& entries, const std::string& id) {
  for (const auto& entry : entries) if (entry.id == id) return &entry;
  return nullptr;
}

bool Requires(const save::WorldState& state, const std::vector<ObjectRequirement>& requirements) {
  for (const auto& requirement : requirements) {
    const auto* object = state.Find(requirement.id);
    if (!object || !object->present || object->stateId != requirement.state) return false;
  }
  return true;
}

std::optional<companion::Character> Person(const std::string& id) {
  if (id == "harley") return companion::Character::Harley;
  if (id == "bartholomeu") return companion::Character::Bartholomeu;
  if (id == "jack") return companion::Character::Jack;
  return std::nullopt;
}

std::string Identity(const WorldDefinition& d, const survival::Tuning& t) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << data::Revision
      << StrategyRevision << QuestionSeconds << JointSeconds << ReportFreshSeconds
      << RescueRuntimeRevision << ' ' << RestrainSeconds << ' ' << ManualReleaseSeconds << ' ' << ToolReleaseSeconds
      << ' ' << SelfReleaseSeconds << ' ' << CaptureTransferSeconds
      << InteractionRuntimeRevision << SpecialRuntimeRevision << CombatRuntimeRevision << HunterRuntimeRevision << partydata::Revision << director::Content().revision << std::quoted(d.levelId);
  out << " NODES " << d.nodes.size();
  for (const auto& node : d.nodes) out << std::quoted(node);
  out << " OBJECTS " << d.objects.size();
  for (const auto& o : d.objects) {
    const auto& s = o.initial;
    out << std::quoted(o.id) << std::quoted(o.archetype) << std::quoted(s.stateId)
        << ' ' << s.positionMeters.x << ' ' << s.positionMeters.y << ' ' << s.positionMeters.z
        << ' ' << s.rotationDegrees.x << ' ' << s.rotationDegrees.y << ' ' << s.rotationDegrees.z << ' ' << s.present;
  }
  out << " DOORS " << d.doors.size();
  for (const auto& door : d.doors)
    out << std::quoted(door.id) << std::quoted(door.fromNode) << std::quoted(door.toNode)
        << std::quoted(door.circuitId) << ' ' << static_cast<int>(door.power);
  out << " CAMERAS " << d.cameras.size();
  for (const auto& camera : d.cameras)
    out << std::quoted(camera.id) << std::quoted(camera.atNode) << std::quoted(camera.viewNode) << std::quoted(camera.circuitId);
  out << " HIDES " << d.hides.size();
  for (const auto& hide : d.hides) out << std::quoted(hide.id) << std::quoted(hide.atNode);
  out << " CIRCUITS " << d.circuitNodes.size();
  for (const auto& [id, node] : d.circuitNodes) out << std::quoted(id) << std::quoted(node);
  const auto requirements = [&](const auto& entries) {
    out << " REQUIRES " << entries.size();
    for (const auto& entry : entries) out << std::quoted(entry.id) << std::quoted(entry.state);
  };
  out << " PICKUPS " << d.pickups.size();
  for (const auto& p : d.pickups) {
    out << std::quoted(p.id) << std::quoted(p.node) << std::quoted(p.item) << ' ' << p.quantity << ' '
        << (p.onCollect ? static_cast<int>(*p.onCollect) : -1);
    requirements(p.requirements);
  }
  out << " USES " << d.uses.size();
  for (const auto& u : d.uses) {
    out << std::quoted(u.id) << std::quoted(u.node) << std::quoted(u.object) << std::quoted(u.before)
        << std::quoted(u.after) << std::quoted(u.item) << ' ' << u.quantity << ' ' << u.consume << ' '
        << (u.runEvent ? static_cast<int>(*u.runEvent) : -1);
    requirements(u.requirements);
  }
  out << " UNLOCKS " << d.unlocks.size();
  for (const auto& u : d.unlocks)
    out << std::quoted(u.id) << std::quoted(u.node) << std::quoted(u.door) << std::quoted(u.item)
        << ' ' << u.quantity << ' ' << u.consume;
  out << " TRIGGERS " << d.triggers.size();
  for (const auto& e : d.triggers) {
    out << std::quoted(e.id) << std::quoted(e.node) << ' ' << static_cast<int>(e.event);
    requirements(e.requirements);
  }
  out << " DESTRUCTIBLES " << d.destructibles.size();
  for (const auto& object : d.destructibles)
    out << std::quoted(object.id) << std::quoted(object.node) << std::quoted(object.before)
        << std::quoted(object.after) << ' ' << object.gunshot << ' ' << object.explosion;
  out << " FINALE " << bool(d.finale);
  if (d.finale) {
    const auto& f = *d.finale;
    out << std::quoted(f.confrontationNode) << std::quoted(f.confrontationExit) << std::quoted(f.detonatorNode)
        << std::quoted(f.collapseShelter) << ' ' << f.confrontationSeconds << ' ' << f.opportunitySeconds
        << ' ' << f.plantSeconds << ' ' << f.collapseSeconds << ' ' << f.minimumCompanions << ' ' << f.minimumTrust;
    out << " SITES " << f.sites.size(); for (const auto& site : f.sites) out << std::quoted(site.id) << std::quoted(site.node);
    out << " SUPPLIES " << f.supplies.size(); for (const auto& supply : f.supplies) out << std::quoted(supply.id) << std::quoted(supply.node) << std::quoted(supply.item) << ' ' << supply.quantity;
  }
  out << " APPEARANCES " << d.appearances.size();
  for (const auto& a : d.appearances) out << std::quoted(a.id) << std::quoted(a.node) << std::quoted(a.block) << ' ' << a.modes << ' ' << a.minimumAct << ' ' << a.tickets;
  out << " WEDGES " << d.wedges.size();
  for (const auto& w : d.wedges) out << std::quoted(w.id) << std::quoted(w.item) << ' ' << w.resistanceSeconds;
  out << " VARIATIONS " << d.variations.size();
  for (const auto& v : d.variations) {
    out << std::quoted(v.id) << std::quoted(v.node) << std::quoted(v.speaker);
    for (const auto& text : v.ambient) out << std::quoted(text);
    for (const auto& text : v.lines) out << std::quoted(text);
  }
  out << " TUNING " << t.maxStamina << ' ' << t.sprintDrainPerSecond << ' ' << t.restRecoveryPerSecond
      << ' ' << t.walkRecoveryFraction << ' ' << t.moderateDrainMultiplier << ' ' << t.severeDrainMultiplier
      << ' ' << t.moderateRecoveryMultiplier << ' ' << t.severeRecoveryMultiplier << ' ' << t.exhaustionReleaseFraction
      << ' ' << t.bleedingPerSecond << ' ' << t.collapseBloodLoss << ' ' << t.fatalBloodLoss << ' ' << t.treatmentBloodRecovery;
  return detail::Hash(out.str());
}

void Validate(const WorldDefinition& d, save::WorldState& state, const survival::Inventory& inventory) {
  const auto node = [&](const std::string& id) {
    if (!d.nodes.count(id)) throw std::invalid_argument("Unknown authored node: " + id);
  };
  const auto object = [&](const std::string& id, const std::string& type) {
    for (const auto& o : d.objects) if (o.id == id && (type.empty() || o.archetype == type)) return;
    throw std::invalid_argument("Unknown authored object/type: " + id);
  };
  const auto item = [&](const std::string& id, int quantity) {
    if (!inventory.CanAdd(id, quantity)) {
      auto empty = inventory.CaptureSnapshot();
      empty.contents.clear();
      survival::Inventory catalog(empty.capacitySlots, empty.definitions);
      if (!catalog.CanAdd(id, quantity)) throw std::invalid_argument("Invalid authored item/quantity");
    }
  };
  const auto event = [](rules::Event e) {
    if (e < rules::Event::SafeOpened || e > rules::Event::JoeDied)
      throw std::invalid_argument("Unknown authored route event");
  };
  const auto requirements = [&](const auto& entries) {
    for (const auto& r : entries) { object(r.id, ""); if (!detail::Token(r.state)) throw std::invalid_argument("Invalid required state"); }
  };
  std::set<std::string> ids;
  const auto unique = [&](const std::string& id) {
    if (!detail::Token(id) || !ids.insert(id).second) throw std::invalid_argument("Invalid/duplicate binding");
  };
  if (d.nodes.empty()) throw std::invalid_argument("Empty world nodes");
  for (const auto& n : d.nodes) if (!detail::Token(n)) throw std::invalid_argument("Invalid node ID");
  for (const auto& door : d.doors) {
    node(door.fromNode); node(door.toNode);
    if (state.Find(door.id) && state.Find(door.id)->stateId == "wedged") throw std::invalid_argument("Wedge needs a confirmed installation");
    if (door.power < world::DoorPower::Manual || door.power > world::DoorPower::ReleasesWithoutPower)
      throw std::invalid_argument("Unknown door power behavior");
  }
  for (const auto& camera : d.cameras) { node(camera.atNode); node(camera.viewNode); }
  for (const auto& hide : d.hides) node(hide.atNode);
  for (const auto& [id, n] : d.circuitNodes) { node(n); object(id, "circuit"); }
  for (const auto& p : d.pickups) {
    unique(p.id); node(p.node); object(p.id, "pickup"); item(p.item, p.quantity); requirements(p.requirements);
    if (p.onCollect) event(*p.onCollect);
    const auto* initial = state.Find(p.id);
    if (!initial->present || initial->stateId != "available") throw std::invalid_argument("Pickup initial state must be available");
  }
  for (const auto& u : d.uses) {
    unique(u.id); node(u.node); object(u.object, "interaction"); requirements(u.requirements);
    if (!detail::Token(u.before) || !detail::Token(u.after) || u.before == u.after)
      throw std::invalid_argument("Invalid interaction states");
    const auto* initial = state.Find(u.object);
    if (!initial->present || (initial->stateId != u.before && initial->stateId != u.after))
      throw std::invalid_argument("Invalid interaction initial state");
    if (u.item.empty()) { if (u.quantity || u.consume) throw std::invalid_argument("Interaction cost lacks item"); }
    else item(u.item, u.quantity);
    if (u.runEvent) event(*u.runEvent);
  }
  for (const auto& u : d.unlocks) {
    unique(u.id); node(u.node); object(u.door, "door"); item(u.item, u.quantity);
    const auto* door = Find(d.doors, u.door);
    if (!door || (u.node != door->fromNode && u.node != door->toNode) ||
        !inventory.IsCategory(u.item, survival::ItemCategory::Utility)) throw std::invalid_argument("Invalid unlock binding");
  }
  for (const auto& t : d.triggers) {
    unique(t.id); if (t.node != "*") node(t.node); event(t.event); requirements(t.requirements);
  }
  std::set<std::string> destructibles;
  for (const auto& target : d.destructibles) {
    node(target.node); object(target.id, "destructible");
    if (!destructibles.insert(target.id).second || !detail::Token(target.before) ||
        !detail::Token(target.after) || target.before == target.after || (!target.gunshot && !target.explosion) ||
        state.Find(target.id)->stateId != target.before || !state.Find(target.id)->present)
      throw std::invalid_argument("Invalid combat destructible binding");
    for (const auto& use : d.uses) if (use.object == target.id)
      throw std::invalid_argument("Combat target overlaps required interaction");
  }
  if (d.finale) {
    const auto& f = *d.finale;
    node(f.confrontationNode); node(f.confrontationExit); node(f.detonatorNode); node(f.collapseShelter);
    for (double duration : {f.confrontationSeconds, f.opportunitySeconds, f.plantSeconds, f.collapseSeconds})
      if (!std::isfinite(duration) || duration <= 0 || duration > 60) throw std::invalid_argument("Invalid finale duration");
    if (f.minimumCompanions < 2 || f.minimumCompanions > 3 || !std::isfinite(f.minimumTrust) ||
        f.minimumTrust < 0 || f.minimumTrust > 100 || f.sites.size() < 3 || f.sites.size() > 16 || f.supplies.size() > 32)
      throw std::invalid_argument("Invalid finale preparation");
    std::set<std::string> sectors;
    for (const auto& site : f.sites) {
      unique(site.id); node(site.node); object(site.id, "rare_site");
      if (!sectors.insert(site.node).second || !state.Find(site.id)->present || state.Find(site.id)->stateId != "empty")
        throw std::invalid_argument("Invalid distinct charge site");
    }
    for (const auto& supply : f.supplies) {
      unique(supply.id); node(supply.node); object(supply.id, "rare_supply"); item(supply.item, supply.quantity);
      if ((supply.item != "structural_charge" && supply.item != "structural_detonator") ||
          !state.Find(supply.id)->present || state.Find(supply.id)->stateId != "available") throw std::invalid_argument("Invalid rare supply");
    }
  }
  if (d.appearances.size() > 16) throw std::invalid_argument("Too many appearances");
  for (const auto& a : d.appearances) {
    unique(a.id); node(a.node);
    const auto block = director::Content().blocks.find(a.block);
    if (!a.modes || a.modes > 15 || a.minimumAct < 1 || a.minimumAct > 5 || a.tickets > 1000 || !a.tickets ||
        a.block.rfind("SP_", 0) != 0 || block == director::Content().blocks.end() || !block->second.nodes.count(a.node) ||
        block->second.modes != a.modes || !block->second.requirements.count("appearance:" + a.id))
      throw std::invalid_argument("Invalid authored appearance");
  }
  if (d.wedges.size() > d.doors.size() || d.variations.size() > 32) throw std::invalid_argument("Too many optional interactions");
  std::set<std::string> wedgeIds;
  for (const auto& w : d.wedges) {
    const auto* door = Find(d.doors, w.id);
    item(w.item, 1);
    if (!door || door->power != world::DoorPower::Manual || !wedgeIds.insert(w.id).second ||
        !inventory.IsCategory(w.item, survival::ItemCategory::Utility) || !std::isfinite(w.resistanceSeconds) ||
        w.resistanceSeconds < 1 || w.resistanceSeconds > 30 || state.Find(w.id)->stateId == "wedged")
      throw std::invalid_argument("Invalid recoverable wedge");
  }
  for (const auto& v : d.variations) {
    unique(v.id); node(v.node);
    if (!Person(v.speaker) || v.ambient.size() != 3 || v.lines.size() != 3) throw std::invalid_argument("Invalid optional variation");
    for (const auto& texts : {v.ambient, v.lines}) for (const auto& text : texts)
      if (text.empty() || text.size() > 1000 || text.find('\n') != std::string::npos) throw std::invalid_argument("Invalid optional text");
  }
  world::WorldCore devices(state, d.doors, d.cameras, d.hides);
  (void)devices;
}
} // namespace

CampaignCore::CampaignCore(rules::Difficulty difficulty, std::uint64_t seed)
    : CampaignCore(difficulty, data::Hospital(), seed) {}
CampaignCore::CampaignCore(rules::Difficulty difficulty, WorldDefinition definition,
                           std::uint64_t seed, survival::Tuning tuning)
    : definition_(std::make_shared<const WorldDefinition>(std::move(definition))),
      state_(std::make_unique<State>(difficulty, *definition_, seed, tuning)),
      contentIdentity_(Identity(*definition_, tuning)) {
  Validate(*definition_, state_->world, state_->inventory);
}
CampaignCore::~CampaignCore() = default;
CampaignCore::CampaignCore(CampaignCore&&) noexcept = default;
CampaignCore& CampaignCore::operator=(CampaignCore&&) noexcept = default;
double CampaignCore::Now() const { return state_->world.NowSeconds(); }
bool CampaignCore::Paused() const { return state_->paused; }
void CampaignCore::SetPaused(bool paused) { state_->paused = paused; }
rules::Ending CampaignCore::Ending() const { return state_->routes.Result(); }
survival::InventorySnapshot CampaignCore::Inventory() const { return state_->inventory.CaptureSnapshot(); }
survival::SurvivalSnapshot CampaignCore::Survival() const { return state_->survival.CaptureSnapshot(); }
companion::CompanionSnapshot CampaignCore::Companion(companion::Character person) const { return state_->Person(person).CaptureSnapshot(); }
save::WorldSnapshot CampaignCore::World() const { return state_->world.CaptureSnapshot(); }
save::NarrativeSnapshot CampaignCore::Narrative() const { return state_->narrative.CaptureSnapshot(); }
hunter::HunterSnapshot CampaignCore::Hunter() const { return state_->hunter.CaptureSnapshot(); }
std::string CampaignCore::HidingPlace() const { return state_->hidingPlace; }
std::size_t CampaignCore::AppliedActionCount() const { return state_->receipts.size(); }
std::optional<world::Feed> CampaignCore::Camera(const std::string& id) const {
  if (!Find(definition_->cameras, id)) return std::nullopt;
  world::WorldCore devices(state_->world, definition_->doors, definition_->cameras, definition_->hides);
  return devices.ReadCamera(id);
}

TickResult CampaignCore::Tick(double seconds, survival::Activity activity,
                              const std::map<std::string, survival::Activity>& companionActivities) {
  if (!std::isfinite(seconds) || seconds < 0 || !std::isfinite(Now() + seconds)) return {Status::InvalidTime, 0, {}};
  if (activity < survival::Activity::Rest || activity > survival::Activity::Sprint ||
      (!state_->hidingPlace.empty() && activity != survival::Activity::Rest)) return {Status::InvalidAction, 0, {}};
  for (const auto& [name, requested] : companionActivities) {
    const auto found = state_->bodies.find(name);
    if (found == state_->bodies.end() || requested < survival::Activity::Rest || requested > survival::Activity::Sprint ||
        (!found->second.hidingPlace.empty() && requested != survival::Activity::Rest)) return {Status::InvalidAction, 0, {}};
  }
  if (Paused()) return {Status::Paused, 0, {}};
  if (Ending() != rules::Ending::None) return {Status::Finished, 0, {}};
  if (RescueBusy(*state_,"joe") && activity!=survival::Activity::Rest) return {Status::MissingRequirement,0,{}};
  for (const auto& [actor, requested] : companionActivities)
    if (RescueBusy(*state_,actor) && requested!=survival::Activity::Rest) return {Status::MissingRequirement,0,{}};
  if ((state_->special.phase == SpecialPhase::Confronting || state_->special.phase == SpecialPhase::Collapsing) &&
      Now() >= state_->special.boundaryAt) return {Status::AwaitingPhysics, 0, {}};
  for (const auto& [serial, item] : state_->combat.thrown) {
    (void)serial;
    if (Now() >= item.detonateAt) return {Status::AwaitingPhysics, 0, {}};
  }
  auto next = std::make_unique<State>(*state_);
  const auto before = next->survival.CaptureSnapshot();
  double elapsed = seconds;
  if (next->special.phase == SpecialPhase::Confronting || next->special.phase == SpecialPhase::Collapsing)
    elapsed = std::min(elapsed, next->special.boundaryAt - Now());
  for (const auto& [serial, item] : next->combat.thrown) {
    (void)serial;
    elapsed = std::min(elapsed, item.detonateAt - Now());
  }
  if (next->combat.holdingBreath) {
    const auto p = CombatProfile(next->routes.DifficultyLevel());
    if (activity == survival::Activity::Sprint) elapsed = 0;
    else elapsed = std::min(elapsed, next->combat.oxygen / (p.oxygenDrain * next->survival.CurrentEffects().breathingLoudnessMultiplier));
  }
  bool fatalBoundary = false;
  if (before.bleeding) {
    const double remaining = (before.tuning.fatalBloodLoss - before.bloodLoss) / before.tuning.bleedingPerSecond;
    if (remaining <= elapsed) { elapsed = remaining; fatalBoundary = true; }
  }
  const bool forcedBreathRelease = AdvanceCombat(*next, elapsed, activity);
  auto result = next->survival.Advance(elapsed, activity);
  if (fatalBoundary && !result.becameDead) {
    // Exact boundary can round a few ulps below fatal loss. Do not run another
    // frame or advance past death merely to compensate for floating point.
    auto atBoundary = next->survival.CaptureSnapshot();
    result.becameCollapsed = result.becameCollapsed || !atBoundary.collapsed;
    atBoundary.bloodLoss = before.tuning.fatalBloodLoss;
    atBoundary.collapsed = atBoundary.dead = true;
    if (!next->survival.RestoreSnapshot(atBoundary)) throw std::logic_error("Invalid fatal boundary");
    result.becameDead = true;
  }
  AdvanceHunter(*next, Now() + elapsed);
  next->AdvanceClocks(Now() + elapsed);
  if (next->special.phase == SpecialPhase::Opportunity && Now() + elapsed >= next->special.boundaryAt) {
    next->special.phase = SpecialPhase::Failed; next->special.boundaryAt = -1;
  }
  AdvanceCompanions(*next, elapsed, companionActivities);
  if (result.becameDead && !next->routes.Apply(rules::Event::JoeDied, Now() + elapsed))
    throw std::logic_error("Confirmed death rejected by campaign");
  next->SyncEnding();
  ReconcileCombat(*next);
  state_.swap(next);
  return {Status::Applied, elapsed, result, forcedBreathRelease};
}

bool CampaignCore::StaticValid(const Action& a) const {
  if (!detail::Token(a.eventId) || !detail::Token(a.target) || !definition_->nodes.count(a.node) ||
      !std::isfinite(a.atSeconds) || a.atSeconds < 0 ||
      a.kind < ActionKind::Collect || a.kind > ActionKind::SubmitKeypad || a.presentationSerial || !a.cueId.empty()) return false;
  if (a.kind == ActionKind::Injury) return a.target == "joe" && a.argument >= 1 && a.argument <= 3;
  if (a.kind == ActionKind::Circuit) return definition_->circuitNodes.count(a.target) && (a.argument == 0 || a.argument == 1);
  if (a.argument) return false;
  if (a.kind == ActionKind::SubmitKeypad)
    return a.target.size() == 6 && std::all_of(a.target.begin(), a.target.end(), [](char c) { return c >= '0' && c <= '9'; }) && Find(definition_->uses, "safe_code_accepted");
  switch (a.kind) {
    case ActionKind::Collect: case ActionKind::Drop: return Find(definition_->pickups, a.target);
    case ActionKind::Treat: case ActionKind::Stabilize: return state_->inventory.IsCategory(a.target, survival::ItemCategory::Medical);
    case ActionKind::Use: return a.target != "safe_code_accepted" && Find(definition_->uses, a.target);
    case ActionKind::UnlockDoor: return Find(definition_->unlocks, a.target);
    case ActionKind::OpenDoor: case ActionKind::CloseDoor: return Find(definition_->doors, a.target);
    case ActionKind::EnterHide: case ActionKind::LeaveHide: return Find(definition_->hides, a.target);
    case ActionKind::Trigger: return Find(definition_->triggers, a.target);
    case ActionKind::ShareAmmo: return Person(a.target).has_value();
    default: return false;
  }
}

ActionResult CampaignCore::Apply(const Action& a) {
  if (!StaticValid(a)) return {Status::InvalidAction, {}};
  const auto signature = detail::Signature(a);
  const auto receipt = state_->receipts.find(a.eventId);
  if (receipt != state_->receipts.end()) return {receipt->second == signature ? Status::AlreadyApplied : Status::Conflict, {}};
  if (Paused()) return {Status::Paused, {}};
  if (Ending() != rules::Ending::None) return {Status::Finished, {}};
  if (state_->special.phase == SpecialPhase::Collapsing ||
      (state_->special.phase == SpecialPhase::Confronting && Now() >= state_->special.boundaryAt)) return {Status::AwaitingPhysics, {}};
  if (a.atSeconds != Now()) return {Status::InvalidTime, {}};
  if (!a.physicallyVerified) return {Status::InvalidAction, {}};
  if (!ValidLocation(a) || (!state_->hidingPlace.empty() &&
      a.node != Find(definition_->hides, state_->hidingPlace)->atNode))
    return {Status::MissingRequirement, {}};
  if (state_->receipts.size() + detail::CombatReservedReceipts(state_->combat) + detail::SpecialReservedReceipts(state_->special) + detail::RescueReservedReceipts(state_->rescue) + detail::StrategyReservedReceipts(state_->strategy) >= detail::MaxReceipts) return {Status::LogFull, {}};
  auto next = std::make_unique<State>(*state_);
  const auto result = Execute(*next, a);
  if (result.status != Status::Applied) return result;
  next->SyncEnding();
  next->receipts.emplace(a.eventId, signature);
  ReconcileCombat(*next);
  state_.swap(next);
  return result;
}

bool CampaignCore::ValidLocation(const Action& a) const {
  // StaticValid must run first. This also validates historical receipts on load.
  switch (a.kind) {
    case ActionKind::Collect: case ActionKind::Drop: return a.node == Find(definition_->pickups, a.target)->node;
    case ActionKind::SubmitKeypad: return a.node == Find(definition_->uses, "safe_code_accepted")->node;
    case ActionKind::Use: return a.node == Find(definition_->uses, a.target)->node;
    case ActionKind::UnlockDoor: return a.node == Find(definition_->unlocks, a.target)->node;
    case ActionKind::OpenDoor: case ActionKind::CloseDoor: {
      const auto& door = *Find(definition_->doors, a.target);
      return a.node == door.fromNode || a.node == door.toNode;
    }
    case ActionKind::Circuit: return a.node == definition_->circuitNodes.at(a.target);
    case ActionKind::EnterHide: case ActionKind::LeaveHide: return a.node == Find(definition_->hides, a.target)->atNode;
    case ActionKind::Trigger: {
      const auto& trigger = *Find(definition_->triggers, a.target);
      return trigger.node == "*" || trigger.node == a.node;
    }
    default: return true; // Joe/companion interaction: actual proximity belongs to the adapter.
  }
}

ActionResult CampaignCore::Execute(State& s, const Action& a) {
  if (!RescueLocation(s,"joe",a.node)) return {Status::MissingRequirement,{}};
  const bool captured = a.kind==ActionKind::Trigger && Find(definition_->triggers,a.target)->event==rules::Event::CapturedByHunter;
  if (RescueBusy(s, "joe") && a.kind != ActionKind::Injury &&
      a.kind != ActionKind::Treat && a.kind != ActionKind::Stabilize && !captured) return {Status::MissingRequirement, {}};
  const ActionResult denied{Status::MissingRequirement, {}};
  world::WorldCore devices(s.world, definition_->doors, definition_->cameras, definition_->hides);
  switch (a.kind) {
    case ActionKind::Collect: case ActionKind::Drop: {
      const auto& p = *Find(definition_->pickups, a.target);
      auto object = *s.world.Find(p.id);
      if (a.node != p.node || PickupReserved(s, p.id)) return denied;
      if (a.kind == ActionKind::Collect) {
        if (!object.present || object.stateId != "available" || !Requires(s.world, p.requirements) ||
            !s.inventory.Add(p.item, p.quantity)) return denied;
        if (p.onCollect && !s.routes.Apply(*p.onCollect, a.atSeconds)) return denied;
        object.stateId = "collected"; object.present = false;
      } else {
        if (!CombatItemAvailable(s, p.item, p.quantity)) return denied;
        if (object.present || !s.inventory.Remove(p.item, p.quantity, survival::RemovalReason::Drop)) return denied;
        object.stateId = "available"; object.present = true;
      }
      if (!s.world.SetObject(p.id, object, a.atSeconds)) return denied;
      break;
    }
    case ActionKind::Treat: case ActionKind::Stabilize:
      if (!CombatItemAvailable(s, a.target, 1) || !s.inventory.Remove(a.target, 1, survival::RemovalReason::Consume) ||
          !(a.kind == ActionKind::Treat ? s.survival.Treat() : s.survival.Stabilize())) return denied;
      break;
    case ActionKind::Use: {
      const auto& u = *Find(definition_->uses, a.target);
      auto object = *s.world.Find(u.object);
      if (a.node != u.node || !object.present || object.stateId != u.before || !Requires(s.world, u.requirements) ||
          (!u.item.empty() && !CombatItemAvailable(s, u.item, u.quantity))) return denied;
      if (u.consume && !s.inventory.Remove(u.item, u.quantity, survival::RemovalReason::Consume)) return denied;
      if (u.runEvent && !s.routes.Apply(*u.runEvent, a.atSeconds)) return denied;
      object.stateId = u.after;
      if (!s.world.SetObject(u.object, object, a.atSeconds)) return denied;
      break;
    }
    case ActionKind::UnlockDoor: {
      const auto& u = *Find(definition_->unlocks, a.target);
      if (a.node != u.node || !CombatItemAvailable(s, u.item, u.quantity) || !devices.UnlockDoor(u.door, a.atSeconds)) return denied;
      if (u.consume && !s.inventory.Remove(u.item, u.quantity, survival::RemovalReason::Consume)) return denied;
      break;
    }
    case ActionKind::OpenDoor: case ActionKind::CloseDoor: {
      const auto& door = *Find(definition_->doors, a.target);
      if (a.node != door.fromNode && a.node != door.toNode) return denied;
      if (a.kind == ActionKind::OpenDoor) {
        const auto signal = devices.OpenDoor(a.target, a.atSeconds);
        if (!signal) return denied;
        return {Status::Applied, signal};
      }
      if (!devices.CloseDoor(a.target, a.atSeconds)) return denied;
      break;
    }
    case ActionKind::Circuit: {
      if (a.node != definition_->circuitNodes.at(a.target)) return denied;
      const auto signal = devices.SetCircuit(a.target, a.argument != 0, a.atSeconds);
      if (!signal) return denied;
      return {Status::Applied, signal};
    }
    case ActionKind::EnterHide: case ActionKind::LeaveHide: {
      const auto& hide = *Find(definition_->hides, a.target);
      if (a.node != hide.atNode) return denied;
      if (a.kind == ActionKind::EnterHide) {
        if (!s.hidingPlace.empty() || HideReserved(s, hide.id) || !devices.EnterHide(hide.id, a.atSeconds)) return denied;
        s.hidingPlace = hide.id;
        s.combat.task.reset();
      } else {
        if (s.hidingPlace != hide.id || !devices.LeaveHide(hide.id, a.atSeconds)) return denied;
        s.hidingPlace.clear();
      }
      break;
    }
    case ActionKind::Trigger: {
      const auto& trigger = *Find(definition_->triggers, a.target);
      if ((trigger.node != "*" && trigger.node != a.node) || !Requires(s.world, trigger.requirements)) return denied;
      for (const auto& [id, signature] : s.receipts) {
        (void)id;
        Action old;
        if (detail::ParseSignature(signature, old) && old.kind == ActionKind::Trigger && old.target == a.target) return denied;
      }
      if (trigger.event == rules::Event::DawnReached && (!s.profile.dawnSeconds || a.atSeconds < *s.profile.dawnSeconds)) return denied;
      if (trigger.event == rules::Event::HunterAbandonedSearch &&
          (s.hidingPlace.empty() || s.hunter.CurrentState() != hunter::State::Patrol)) return denied;
      if (!s.routes.Apply(trigger.event, a.atSeconds)) return denied;
      if (trigger.event == rules::Event::TunnelFound) s.joeKnownRoutes.insert("tunnel");
      if (trigger.event == rules::Event::WindowFound) s.joeKnownRoutes.insert("window");
      break;
    }
    case ActionKind::Injury:
      if (!s.survival.ApplyInjury(static_cast<survival::Injury>(a.argument))) return denied;
      s.combat.task.reset();
      break;
    case ActionKind::ShareAmmo: {
      const auto status = s.narrative.CaptureSnapshot().outcomes.at(a.target);
      auto& body = s.bodies.at(a.target);
      if (status.dead || status.captured || status.escapedAlone || status.together || body.task ||
          !body.inventory.CanAdd("magazine", 1) || !CombatItemAvailable(s, "magazine", 1)) return denied;
      if (!integration::ShareAmmunition(s.inventory, "magazine", s.Person(*Person(a.target)), a.eventId, a.atSeconds)) return denied;
      if (!body.inventory.Add("magazine", 1)) return denied;
      auto story = s.narrative.CaptureSnapshot();
      story.durableFacts.insert("joe_shared_ammo_with_" + a.target);
      if (!s.narrative.RestoreSnapshot(story)) return denied;
      break;
    }
    case ActionKind::SubmitKeypad: {
      auto story = s.narrative.CaptureSnapshot();
      if (s.routes.CaptureSnapshot().safeOpened || story.keypadAttempts >= 100000) return denied;
      ++story.keypadAttempts;
      if (!s.narrative.RestoreSnapshot(story)) return denied;
      if (a.target != director::Content().garageCode) return {Status::Applied, {}, false};
      auto opening = a; opening.kind = ActionKind::Use; opening.target = "safe_code_accepted";
      const auto result = Execute(s, opening);
      if (result.status != Status::Applied) return result;
      return {Status::Applied, {}, true};
    }
    default: return {Status::InvalidAction, {}};
  }
  return {Status::Applied, {}};
}

bool CampaignCore::Hear(const perception::SoundEvent& sound, const perception::ListenerContext& listener) {
  if (Paused() || Ending() != rules::Ending::None || sound.atSeconds != Now()) return false;
  auto next = std::make_unique<State>(*state_);
  try {
    const auto heard = next->hearing.Hear(sound, listener);
    if (!heard || !next->hunter.Observe(*heard)) return false;
  } catch (const std::invalid_argument&) { return false; }
  state_.swap(next); return true;
}

bool CampaignCore::Observe(const hunter::Stimulus& stimulus) {
  if (stimulus.kind == hunter::StimulusKind::ConfirmedHideSpot && !Find(definition_->hides, stimulus.observedId)) return false;
  if (Paused() || Ending() != rules::Ending::None || stimulus.atSeconds != Now()) return false;
  auto next = std::make_unique<State>(*state_);
  if (!next->hunter.Observe(stimulus)) return false;
  state_.swap(next); return true;
}

bool CampaignCore::InspectEvidence(const std::string& id, const std::string& visible,
                                    hunter::Vec3 position, bool verified) {
  const auto* object = state_->world.Find(id);
  if (!verified || !object || !object->present || object->stateId != visible ||
      Paused() || Ending() != rules::Ending::None) return false;
  auto next = std::make_unique<State>(*state_);
  try {
    const auto observed = next->evidence.Inspect(id, visible, position, Now());
    if (observed && !next->hunter.Observe(*observed)) return false;
  } catch (const std::invalid_argument&) { return false; }
  state_.swap(next); return true;
}

bool CampaignCore::Consistent(const State& s) const {
  const double now = s.world.NowSeconds();
  const auto ending = s.routes.Result();
  const bool escaped = ending == rules::Ending::GarageEscape || ending == rules::Ending::RoofEscape ||
                       ending == rules::Ending::TunnelEscape || ending == rules::Ending::WindowEscape || ending == rules::Ending::CoordinatedEscape;
  if (s.routes.CaptureSnapshot().nowSeconds != now || s.hunter.CaptureSnapshot().nowSeconds != now ||
      s.harley.CaptureSnapshot().nowSeconds != now || s.bartholomeu.CaptureSnapshot().nowSeconds != now ||
      s.jack.CaptureSnapshot().nowSeconds != now || s.narrative.CaptureSnapshot().nowSeconds != now ||
      s.receipts.size() > detail::MaxReceipts || s.narrative.CaptureSnapshot().joeEscaped != escaped ||
      !director::ValidPresentation(s.narrative.CaptureSnapshot(), s.presentation) || !NarrativeCoherent(s) || !CompanionCoherent(s) || !HunterCoherent(s) || !CombatCoherent(s) || !SpecialCoherent(s) || !InteractionCoherent(s) || !RescueCoherent(s) || !StrategyCoherent(s) || !ResourceCoherent(s)) return false;
  if (!s.hidingPlace.empty()) {
    if (!Find(definition_->hides, s.hidingPlace)) return false;
    const auto* object = s.world.Find(s.hidingPlace);
    if (!object || !object->present || object->stateId != "occupied") return false;
  }
  for (const auto& p : definition_->pickups) {
    const auto* object = s.world.Find(p.id);
    if (!object || (object->present ? object->stateId != "available" : object->stateId != "collected")) return false;
  }
  for (const auto& u : definition_->uses) {
    const auto* object = s.world.Find(u.object);
    if (!object || !object->present || (object->stateId != u.before && object->stateId != u.after)) return false;
  }
  for (const auto& [id, signature] : s.receipts) {
    if (signature.rfind("Q ", 0) == 0) continue; // Validated by StrategyCoherent.
    if (signature.rfind("R ", 0) == 0) continue; // Validated by RescueCoherent.
    if (signature.rfind("I ", 0) == 0) continue; // Validated by InteractionCoherent.
    if (signature.rfind("S ", 0) == 0) continue; // Validated by SpecialCoherent.
    if (signature.rfind("W ", 0) == 0) continue; // Validated by CombatCoherent.
    if (signature.rfind("H ", 0) == 0) continue; // Validated by HunterCoherent.
    if (signature.rfind("C ", 0) == 0) {
      CompanionCommand c; c.eventId = id;
      if (!detail::ParseCompanion(signature, c) || !CompanionStaticValid(c) || c.atSeconds > now) return false;
      continue;
    }
    Action a; a.eventId = id;
    if (!detail::ParseSignature(signature, a) || a.atSeconds > now) return false;
    if (a.kind <= ActionKind::SubmitKeypad ? (!StaticValid(a) || !ValidLocation(a)) : !NarrativeStaticValid(a)) return false;
    if (a.kind == ActionKind::ShareAmmo && !s.Person(*Person(a.target)).CaptureSnapshot().appliedEvents.count(id)) return false;
  }
  try { auto world = s.world; world::WorldCore devices(world, definition_->doors, definition_->cameras, definition_->hides); }
  catch (const std::invalid_argument&) { return false; }
  return true;
}
} // namespace thug::campaign
