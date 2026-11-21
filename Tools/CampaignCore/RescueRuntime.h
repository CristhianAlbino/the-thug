#pragma once
#include "InteractionRuntime.h"

namespace thug::campaign {
// Prototype timings: physical contact/animation proofs still come from the engine.
inline constexpr const char* RescueRuntimeRevision = "temporary-capture-2";
inline constexpr double RestrainSeconds = 2, ManualReleaseSeconds = 10,
    ToolReleaseSeconds = 4, SelfReleaseSeconds = 12, CaptureTransferSeconds = 8;
enum class RescueOp { BeginContainment, FinishContainment, CancelContainment,
    BeginRelease, BeginSelfRelease, CompleteRelease, CancelRelease, FinalizeCapture };
enum class CapturePhase { Restraining, Contained, Released, Finalized };
struct RescueCommand {
  std::string eventId, actor = "joe", target, node, item;
  RescueOp kind = RescueOp::BeginRelease;
  double atSeconds = 0;
  std::uint64_t serial = 0;
  bool physicallyVerified = false;
};
struct RescueContext {
  // Actual perceived participants, never omniscient party transforms.
  std::map<std::string, director::Actor> actors;
  std::string actualHunterNode;
  bool contactVerified = false, continuousContactVerified = false,
       contactLostVerified = false, openingVerified = false,
       actionCompleted = false, transferCompleted = false,
       joeWitnessedAction = false;
};
struct ReleaseTask {
  std::uint64_t serial = 0, helperHarm = 0, targetHarm = 0;
  std::string helper, item, startEventId;
  double startedAt = 0, readyAt = 0;
  bool self = false, interrupted = false;
};
struct TemporaryCapture {
  CapturePhase phase = CapturePhase::Restraining;
  std::string node, startEventId;
  std::uint64_t serial = 0;
  double startedAt = 0, containedAt = -1;
  std::optional<ReleaseTask> release;
};
struct RescueRuntimeSnapshot {
  std::uint64_t serial = 0;
  std::map<std::string, TemporaryCapture> captures;
};
struct RescueOffer { RescueOp kind; std::string target, item; double seconds; };
} // namespace thug::campaign
