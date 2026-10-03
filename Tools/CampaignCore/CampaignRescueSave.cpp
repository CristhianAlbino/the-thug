#include "CampaignState.h"
#include "RescueIO.h"
#include "CompanionIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
bool Tag(std::istream& in, const char* expected) { std::string tag; return bool(in >> tag) && tag == expected; }
bool Quoted(std::istream& in, std::string& value, bool empty = false) {
  in >> std::ws;
  return in.peek() == '"' && bool(in >> std::quoted(value)) && ((empty && value.empty()) || detail::Token(value));
}
std::string Fingerprint(const RescueRuntimeSnapshot& r) {
  std::ostringstream out; out.imbue(std::locale::classic()); detail::WriteRescueRuntime(out, r); return out.str();
}
}
void CampaignCore::WriteRescue(std::ostream& out, const State& s) const { detail::WriteRescueRuntime(out, s.rescue); }
bool CampaignCore::ReadRescue(std::istream& in, State& s) const {
  RescueRuntimeSnapshot r; std::size_t count;
  if (!Tag(in, "RESCUE") || !(in >> r.serial >> count) || count > 3) return false;
  for (std::size_t i = 0; i < count; ++i) {
    TemporaryCapture t; std::string actor; int phase, release;
    if (!Tag(in, "CAPTURE") || !Quoted(in, actor) || !detail::Person(actor) || !(in >> phase) || phase < 0 || phase > 3 ||
        !Quoted(in, t.node) || !Quoted(in, t.startEventId) || !(in >> t.serial >> t.startedAt >> t.containedAt >> release) ||
        release < 0 || release > 1) return false;
    t.phase = static_cast<CapturePhase>(phase);
    if (release) {
      ReleaseTask a; int self, interrupted;
      if (!Tag(in, "RELEASE") || !(in >> a.serial >> a.helperHarm >> a.targetHarm) || !Quoted(in, a.helper) ||
          !Quoted(in, a.item, true) || !Quoted(in, a.startEventId) || !(in >> a.startedAt >> a.readyAt >> self >> interrupted) ||
          self < 0 || self > 1 || interrupted < 0 || interrupted > 1) return false;
      a.self = self != 0; a.interrupted = interrupted != 0; t.release = std::move(a);
    }
    if (!r.captures.emplace(actor, std::move(t)).second) return false;
  }
  s.rescue = std::move(r); return true;
}
bool CampaignCore::RescueCoherent(const State& s) const {
  struct Entry { RescueCommand command; bool witnessed; std::uint64_t helper, target; };
  std::map<std::uint64_t, Entry> ordered;
  const double now = s.world.NowSeconds();
  for (const auto& [id, signature] : s.receipts) if (signature.rfind("R ", 0) == 0) {
    Entry e; std::uint64_t order; e.command.eventId = id;
    if (!detail::ParseRescue(signature, e.command, order, e.witnessed, e.helper, e.target) ||
        !RescueStaticValid(e.command) || !order || e.command.atSeconds > now || !ordered.emplace(order, e).second) return false;
    const auto& c = e.command;
    const double after = std::nextafter(c.atSeconds, std::numeric_limits<double>::infinity());
    if ((c.actor == "hunter" ? e.helper != 0 :
        (e.helper < RescueHarmCount(s, c.actor, c.atSeconds) || e.helper > RescueHarmCount(s, c.actor, after))) ||
        e.target < RescueHarmCount(s, c.target, c.atSeconds) || e.target > RescueHarmCount(s, c.target, after)) return false;
  }
  if (ordered.size() != s.rescue.serial) return false;
  auto expected = std::make_unique<State>(s); expected->rescue = {};
  auto& r = expected->rescue;
  double previousAt = 0;
  for (const auto& [order, e] : ordered) {
    const auto& c = e.command;
    if (order != ++r.serial || c.atSeconds < previousAt) return false;
    previousAt = c.atSeconds;
    auto found = r.captures.find(c.target);
    if (c.kind == RescueOp::BeginContainment) {
      if (found != r.captures.end() && found->second.phase != CapturePhase::Released) return false;
      for (const auto& [actor, t] : r.captures) { (void)actor; if (t.phase == CapturePhase::Restraining) return false; }
      for (auto& [actor,t] : r.captures) {
        (void)actor;
        if (t.release && t.release->helper==c.target) {
          if (t.node!=c.node) return false;
          t.release.reset();
        }
      }
      r.captures[c.target] = {CapturePhase::Restraining, c.node, c.eventId, order, c.atSeconds, -1, {}};
      continue;
    }
    if (found == r.captures.end() || found->second.node != c.node) return false;
    auto& t = found->second;
    switch (c.kind) {
      case RescueOp::FinishContainment:
        if (t.phase != CapturePhase::Restraining || c.serial != t.serial || c.atSeconds < t.startedAt + RestrainSeconds) return false;
        t.phase = CapturePhase::Contained; t.containedAt = c.atSeconds; break;
      case RescueOp::CancelContainment:
        if (t.phase != CapturePhase::Restraining || c.serial != t.serial) return false;
        t.phase = CapturePhase::Released; break;
      case RescueOp::BeginRelease: case RescueOp::BeginSelfRelease: {
        if (t.phase != CapturePhase::Contained || t.release) return false;
        const bool self = c.kind == RescueOp::BeginSelfRelease;
        for (const auto& [actor, other] : r.captures) {
          if (!self && actor == c.actor && (other.phase == CapturePhase::Restraining || other.phase == CapturePhase::Contained)) return false;
          if (other.release && other.release->helper == c.actor) return false;
        }
        t.release = ReleaseTask{order, e.helper, e.target, c.actor, c.item, c.eventId, c.atSeconds,
            c.atSeconds + (self ? SelfReleaseSeconds : c.item.empty() ? ManualReleaseSeconds : ToolReleaseSeconds), self, false};
        break;
      }
      case RescueOp::CompleteRelease: case RescueOp::CancelRelease:
        if (t.phase != CapturePhase::Contained || !t.release || c.serial != t.release->serial || c.actor != t.release->helper) return false;
        if (c.kind == RescueOp::CompleteRelease) {
          if (c.atSeconds < t.release->readyAt || e.helper != t.release->helperHarm || e.target != t.release->targetHarm) return false;
          t.phase = CapturePhase::Released;
        }
        t.release.reset(); break;
      case RescueOp::FinalizeCapture:
        if (t.phase != CapturePhase::Contained || c.serial != t.serial || c.atSeconds < t.containedAt + CaptureTransferSeconds) return false;
        t.phase = CapturePhase::Finalized; t.release.reset(); break;
      case RescueOp::BeginContainment: return false;
    }
  }
  ReconcileRescue(*expected);
  if (Fingerprint(r) != Fingerprint(s.rescue) || s.receipts.size() + detail::CombatReservedReceipts(s.combat) +
      detail::SpecialReservedReceipts(s.special) + detail::RescueReservedReceipts(r) > detail::MaxReceipts) return false;
  const auto story = s.narrative.CaptureSnapshot();
  std::set<std::string> helpers;
  for (const auto& [actor, t] : r.captures) {
    const auto& body = s.bodies.at(actor);
    if (t.phase != CapturePhase::Released && (body.lastNode != t.node || body.task || !body.hidingPlace.empty())) return false;
    if (t.phase == CapturePhase::Finalized && !story.outcomes.at(actor).captured) return false;
    if ((t.phase == CapturePhase::Restraining || t.phase == CapturePhase::Contained) &&
        (story.outcomes.at(actor).captured || story.outcomes.at(actor).together || story.outcomes.at(actor).escapedAlone)) return false;
    if (t.release && !t.release->interrupted) {
      const auto& a = *t.release;
      if (!helpers.insert(a.helper).second || (!a.self && a.helper == actor)) return false;
      if (a.helper == "joe") {
        if (!s.hidingPlace.empty() || s.combat.task || s.special.plant ||
            s.special.phase == SpecialPhase::Confronting || s.special.phase == SpecialPhase::Opportunity) return false;
        if (!a.item.empty() && s.inventory.Quantity(a.item) < 1) return false;
      } else {
        const auto& b = s.bodies.at(a.helper);
        if (s.special.recruited.count(a.helper) &&
            (s.special.phase==SpecialPhase::Confronting || s.special.phase==SpecialPhase::Opportunity)) return false;
        if (b.task || !b.hidingPlace.empty() || b.lastNode!=t.node || (!a.item.empty() && b.inventory.Quantity(a.item) < 1)) return false;
      }
    }
  }
  return true;
}
} // namespace thug::campaign
