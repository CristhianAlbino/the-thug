#pragma once

#include "WorldDefinition.h"
#include "../SaveCore/StorySave.h"

#include <cstdint>
#include <iosfwd>
#include <memory>

namespace thug::campaign {

enum class ActionKind {
  Collect, Drop, Treat, Stabilize, Use, UnlockDoor, OpenDoor, CloseDoor,
  Circuit, EnterHide, LeaveHide, Trigger, Injury, ShareAmmo
};

struct Action {
  std::string eventId;
  ActionKind kind;
  std::string target;
  std::string node;
  double atSeconds = 0;
  int argument = 0; // Circuit: 0/1; Injury: the authored Injury enum value.
  // Supplied only after reachability, actual interaction and event-specific
  // evidence were checked by the world adapter. A UI click is insufficient.
  bool physicallyVerified = false;
};

enum class Status {
  Applied, AlreadyApplied, Paused, Finished, InvalidTime,
  InvalidAction, MissingRequirement, Conflict, LogFull
};

struct ActionResult {
  Status status;
  std::optional<world::WorldSignal> signal;
};

struct TickResult {
  Status status;
  double advancedSeconds = 0;
  survival::AdvanceResult survival;
};

// Sole mutable owner of the pure campaign. All public views are value copies;
// successful commands swap a fully staged state, rejected commands swap none.
// Single simulation thread. This is not an Unreal actor/navigation controller.
class CampaignCore {
 public:
  explicit CampaignCore(rules::Difficulty difficulty, std::uint64_t seed = 1);
  CampaignCore(rules::Difficulty difficulty, WorldDefinition definition,
               std::uint64_t seed = 1, survival::Tuning tuning = {});
  ~CampaignCore();
  CampaignCore(CampaignCore&&) noexcept;
  CampaignCore& operator=(CampaignCore&&) noexcept;
  CampaignCore(const CampaignCore&) = delete;
  CampaignCore& operator=(const CampaignCore&) = delete;

  double Now() const;
  bool Paused() const;
  void SetPaused(bool paused);
  rules::Ending Ending() const;
  TickResult Tick(double seconds, survival::Activity activity);
  ActionResult Apply(const Action& action);
  // Sensor samples are not durable action receipts (footsteps can be frequent).
  bool Hear(const perception::SoundEvent&, const perception::ListenerContext&);
  bool Observe(const hunter::Stimulus&);
  bool InspectEvidence(const std::string& objectId, const std::string& visibleState,
                       hunter::Vec3 observedPosition, bool lineOfSightVerified);

  survival::InventorySnapshot Inventory() const;
  survival::SurvivalSnapshot Survival() const;
  companion::CompanionSnapshot Companion(companion::Character person) const;
  save::WorldSnapshot World() const;
  save::NarrativeSnapshot Narrative() const;
  hunter::HunterSnapshot Hunter() const;
  std::optional<world::Feed> Camera(const std::string& cameraId) const;
  std::string HidingPlace() const;
  std::size_t AppliedActionCount() const;

  // Runtime v1 adds pause, Joe's hiding occupancy and the transaction ledger
  // around StorySave. Legacy core/story files require an explicit migration.
  save::Result Save(std::ostream&) const;
  save::Result Load(std::istream&);

 private:
  struct State;
  std::shared_ptr<const WorldDefinition> definition_;
  std::unique_ptr<State> state_;
  std::string contentIdentity_;

  bool StaticValid(const Action&) const;
  bool ValidLocation(const Action&) const;
  bool Consistent(const State&) const;
  ActionResult Execute(State&, const Action&);
};

} // namespace thug::campaign
