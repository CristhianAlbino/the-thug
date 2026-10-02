#pragma once

#include "WorldDefinition.h"
#include "../SaveCore/StorySave.h"
#include "../DirectorCore/DirectorCore.h"
#include "CompanionRuntime.h"

#include <cstdint>
#include <iosfwd>
#include <memory>

namespace thug::campaign {

enum class ActionKind {
  Collect, Drop, Treat, Stabilize, Use, UnlockDoor, OpenDoor, CloseDoor,
  Circuit, EnterHide, LeaveHide, Trigger, Injury, ShareAmmo, SubmitKeypad,
  BeginScene, SceneCue, CancelScene, BeginBanter, BanterCue, CancelBanter,
  InspectDocument, InspectAudio, WitnessClue, RememberFact, AdvanceAct
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
  std::uint64_t presentationSerial = 0;
  std::string cueId{};
};

enum class Status {
  Applied, AlreadyApplied, Paused, Finished, InvalidTime,
  InvalidAction, MissingRequirement, Conflict, LogFull
};

struct ActionResult {
  Status status;
  std::optional<world::WorldSignal> signal;
  std::optional<bool> acceptedCode{};
};

struct TickResult {
  Status status;
  double advancedSeconds = 0;
  survival::AdvanceResult survival;
};

struct CompanionResult {
  Status status;
  std::optional<CompanionTask> task{};
  std::optional<world::WorldSignal> worldSignal{};
  std::optional<CompanionSignal> actorSignal{};
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
  TickResult Tick(double seconds, survival::Activity activity,
                  const std::map<std::string, survival::Activity>& companionActivities = {});
  ActionResult Apply(const Action& action);
  CompanionResult ApplyCompanion(const CompanionCommand&, const CompanionContext&);
  std::vector<CompanionOffer> CompanionOffers(const std::string& actor, const std::string& actualNode, const CompanionContext&) const;
  CompanionRuntimeSnapshot CompanionRuntime(companion::Character person) const;
  std::set<std::string> JoeKnownRoutes() const;
  // Observation must describe the current clock/act/difficulty and actual
  // physical context. Presentation ACK confirms a heard line/completed gesture.
  ActionResult ApplyNarrative(const Action&, const director::Observation&);
  std::optional<director::Presentation> CurrentPresentation(const director::Observation&, const std::string& locale = "pt-BR") const;
  std::vector<director::Cue> JournalText(const std::string&, const std::string& locale = "pt-BR") const;
  std::vector<director::Cue> EpilogueText(const std::string& locale = "pt-BR") const;
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

  // Runtime v3 includes individual bodies, inventories, knowledge and pending tasks
  // around StorySave. Legacy core/story files require an explicit migration.
  save::Result Save(std::ostream&) const;
  save::Result Load(std::istream&);
  save::Result ValidateCheckpoint(std::istream&) const;

 private:
  struct State;
  std::shared_ptr<const WorldDefinition> definition_;
  std::unique_ptr<State> state_;
  std::string contentIdentity_;

  bool StaticValid(const Action&) const;
  bool NarrativeStaticValid(const Action&) const;
  bool NarrativeCoherent(const State&) const;
  bool ValidObservation(const director::Observation&) const;
  director::Observation NarrativeContext(const director::Observation&, const State&) const;
  bool ValidLocation(const Action&) const;
  bool Consistent(const State&) const;
  ActionResult Execute(State&, const Action&);
  bool CompanionStaticValid(const CompanionCommand&) const;
  bool CompanionContextValid(const CompanionCommand&, const CompanionContext&) const;
  bool CompanionCoherent(const State&) const;
  bool PickupReserved(const State&, const std::string&, const std::string& exceptActor = {}) const;
  bool HideReserved(const State&, const std::string&, const std::string& exceptActor = {}) const;
  std::vector<CompanionOffer> Offers(const State&, const std::string&, const std::string&, const CompanionContext&) const;
  CompanionResult ExecuteCompanion(State&, const CompanionCommand&, const CompanionContext&);
  void AdvanceCompanions(State&, double, const std::map<std::string, survival::Activity>&);
  void WriteCompanions(std::ostream&, const State&) const;
  bool ReadCompanions(std::istream&, State&) const;
};

} // namespace thug::campaign
