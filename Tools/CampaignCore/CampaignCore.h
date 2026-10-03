#pragma once

#include "WorldDefinition.h"
#include "../SaveCore/StorySave.h"
#include "../DirectorCore/DirectorCore.h"
#include "CompanionRuntime.h"
#include "HunterRuntime.h"
#include "CombatRuntime.h"
#include "SpecialRuntime.h"
#include "InteractionRuntime.h"
#include "RescueRuntime.h"
#include "StrategyRuntime.h"

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
  InvalidAction, MissingRequirement, Conflict, LogFull, AwaitingPhysics
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
  bool forcedBreathRelease = false;
};

struct CombatResult {
  Status status;
  std::optional<CombatTask> task{};
  std::optional<Shot> shot{};
  std::optional<ThrownItem> thrown{};
  std::optional<perception::SoundEvent> sound{};
  bool hunterInterrupted = false;
};

struct CompanionResult {
  Status status;
  std::optional<CompanionTask> task{};
  std::optional<world::WorldSignal> worldSignal{};
  std::optional<CompanionSignal> actorSignal{};
};

struct HunterResult {
  Status status;
  std::optional<HunterTask> task{};
  std::optional<world::WorldSignal> signal{};
  std::optional<bool> inspectedHideOccupied{};
};

struct InteractionResult {
  Status status;
  bool accepted = false; // A perceived request can be refused without a task.
  std::optional<world::WorldSignal> signal{};
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
  // Frequent identity/LOS sensor updates use bounded memory, not action receipts.
  bool SenseHunter(const HunterCommand&, const HunterContext&);
  HunterResult ApplyHunter(const HunterCommand&, const HunterContext&);
  std::vector<HunterOffer> HunterOffers(const HunterContext&) const;
  HunterRuntimeSnapshot HunterRuntime() const;
  CombatResult ApplyCombat(const CombatCommand&, const CombatContext&);
  CombatRuntimeSnapshot CombatRuntime() const;
  std::vector<CombatOp> CompanionCombatOffers(const std::string&, const CombatContext&) const;
  // Continuous physical tracking is bounded state, not a durable action receipt.
  bool TrackThrown(std::uint64_t serial, const std::string& actualNode,
                   hunter::Vec3 position, hunter::Vec3 velocity, bool physicallyVerified);
  BreathingEffects Breathing() const;
  std::optional<perception::SoundEvent> BreathingSound(hunter::Vec3 actualSource) const;
  bool SmokeOccludes(hunter::Vec3 from, hunter::Vec3 to) const;
  Status ApplySpecial(const SpecialCommand&, const SpecialContext&);
  SpecialRuntimeSnapshot SpecialRuntime() const;
  InteractionResult ApplyInteraction(const InteractionCommand&, const InteractionContext&);
  InteractionRuntimeSnapshot Interactions() const;
  Status ApplyRescue(const RescueCommand&, const RescueContext&);
  RescueRuntimeSnapshot RescueRuntime() const;
  std::vector<RescueOffer> RescueOffers(const std::string& actor, const RescueContext&) const;
  InteractionResult ApplyStrategy(const StrategyCommand&, const StrategyContext&);
  StrategyRuntimeSnapshot StrategyRuntime() const;
  std::vector<KnownReport> KnownReports(const std::string& actor) const;
  std::map<std::string, KnownDrop> KnownDrops(const std::string& actor) const;
  std::vector<AnswerKind> QuestionOptions() const;
  std::vector<HunterPlanOffer> HunterPlanOffers(const StrategyContext&) const;
  std::optional<NightVariation> VariationFor(const std::string&) const;
  std::optional<director::Cue> InteractionCue(const InteractionCommand&) const;
  std::vector<NightRecordEntry> NightRecord() const;
  std::map<std::string, std::string> KnownCompanionFates() const;
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

  // Runtime v9 adds resources, information, cooperation and individual weapons
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
  bool HunterStaticValid(const HunterCommand&) const;
  bool HunterCoherent(const State&) const;
  std::vector<HunterOffer> HunterOffers(const State&, const HunterContext&) const;
  HunterResult ExecuteHunter(State&, const HunterCommand&, const HunterContext&);
  void AdvanceHunter(State&, double) const;
  void WriteHunter(std::ostream&, const State&) const;
  bool ReadHunter(std::istream&, State&) const;
  void WriteCompanions(std::ostream&, const State&) const;
  bool ReadCompanions(std::istream&, State&) const;
  bool CombatStaticValid(const CombatCommand&) const;
  bool CombatCoherent(const State&) const;
  CombatResult ExecuteCombat(State&, const CombatCommand&, const CombatContext&) const;
  bool AdvanceCombat(State&, double, survival::Activity);
  bool CombatItemAvailable(const State&, const std::string&, int quantity) const;
  bool WeaponBusy(const State&, const std::string&) const;
  void ReconcileCombat(State&) const;
  void WriteCombat(std::ostream&, const State&) const;
  bool ReadCombat(std::istream&, State&) const;
  bool SpecialStaticValid(const SpecialCommand&) const;
  bool SpecialCoherent(const State&) const;
  Status ExecuteSpecial(State&, const SpecialCommand&, const SpecialContext&);
  void WriteSpecial(std::ostream&, const State&) const;
  bool ReadSpecial(std::istream&, State&) const;
  bool InteractionStaticValid(const InteractionCommand&) const;
  InteractionResult ExecuteInteraction(State&, const InteractionCommand&, const InteractionContext&);
  bool InteractionCoherent(const State&) const;
  void WriteInteractions(std::ostream&, const State&) const;
  bool ReadInteractions(std::istream&, State&) const;
  bool RestoreInteractions(State&) const;
  bool QuietBanter(const State&, const std::string&) const;
  bool RescueStaticValid(const RescueCommand&) const;
  Status ExecuteRescue(State&, const RescueCommand&, const RescueContext&) const;
  bool RescueCoherent(const State&) const;
  bool RescueBusy(const State&, const std::string&) const;
  bool RescueLocation(const State&, const std::string&, const std::string&) const;
  std::uint64_t RescueHarmCount(const State&, const std::string&, double before = -1) const;
  void ReconcileRescue(State&) const;
  void WriteRescue(std::ostream&, const State&) const;
  bool ReadRescue(std::istream&, State&) const;
  bool StrategyStaticValid(const StrategyCommand&) const;
  InteractionResult ExecuteStrategy(State&, const StrategyCommand&, const StrategyContext&) const;
  bool ReplayStrategy(StrategyRuntimeSnapshot&, const StrategyCommand&, std::uint64_t, bool,
                      std::uint64_t, std::uint64_t) const;
  bool StrategyBusy(const State&, const std::string&) const;
  void ReconcileStrategy(State&) const;
  bool StrategyCoherent(const State&) const;
  bool ResourceCoherent(const State&) const;
  void WriteStrategy(std::ostream&, const State&) const;
  bool ReadStrategy(std::istream&, State&) const;
  bool RestoreStrategy(State&) const;
  bool AdaptiveStaticValid(const StrategyCommand&) const;
  InteractionResult ExecuteAdaptive(State&, const StrategyCommand&, const StrategyContext&) const;
  bool ReplayAdaptive(StrategyRuntimeSnapshot&, const StrategyCommand&, std::uint64_t, std::uint64_t) const;
  void ReconcileAdaptive(State&) const;
  std::vector<HunterPlanOffer> HunterPlanOffers(const State&, const StrategyContext&) const;
};

} // namespace thug::campaign
