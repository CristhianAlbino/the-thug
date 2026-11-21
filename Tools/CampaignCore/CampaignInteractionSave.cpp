#include "CampaignState.h"
#include "InteractionIO.h"
#include "CompanionIO.h"
#include "HunterIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
std::string Fingerprint(const InteractionRuntimeSnapshot& r) {
  std::ostringstream out; out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10) << r.variation << ' ' << r.serial;
  for (const auto& [actor, o] : r.orders) out << " O " << std::quoted(actor) << ' ' << static_cast<int>(o.kind) << ' ' << std::quoted(o.destination) << ' ' << std::quoted(o.eventId) << ' ' << o.expiresAt << ' ' << o.fulfilled;
  for (const auto* times : {&r.quietUntil, &r.threatUntil, &r.gestureAt}) {
    out << " M " << times->size(); for (const auto& [id, at] : *times) out << std::quoted(id) << ' ' << at;
  }
  out << " W " << r.wedges.size();
  for (const auto& [id, w] : r.wedges) out << std::quoted(id) << ' ' << std::quoted(w.item) << ' ' << std::quoted(w.eventId) << ' ' << w.placedAt;
  for (const auto* entries : {&r.ambientSeen, &r.banterHeard, &r.reunions}) {
    out << " S " << entries->size(); for (const auto& id : *entries) out << std::quoted(id);
  }
  for (const auto& [actor, fate] : r.knownFates) out << " F " << std::quoted(actor) << ' ' << static_cast<int>(fate);
  return out.str();
}
}
void CampaignCore::WriteInteractions(std::ostream& out, const State& s) const {
  // All remaining fields are deterministic derivatives of the confirmed ledger.
  // Keeping one authoritative representation prevents divergent copies on load.
  out << "INTERACTIONS " << s.interactions.variation << ' ' << s.interactions.serial << '\n';
}
bool CampaignCore::ReadInteractions(std::istream& in, State& s) const {
  std::string tag; InteractionRuntimeSnapshot r;
  if (!(in >> tag >> r.variation >> r.serial) || tag != "INTERACTIONS" || r.variation > 2 || r.serial > detail::MaxReceipts) return false;
  s.interactions = std::move(r); return true;
}
bool CampaignCore::RestoreInteractions(State& s) const {
  auto& r = s.interactions;
  const auto variation = r.variation; const auto serial = r.serial;
  if (variation > 2 || serial > detail::MaxReceipts) return false;
  r = {}; r.variation = variation; r.serial = serial;
  std::map<std::uint64_t, std::pair<InteractionCommand, bool>> commands;
  std::map<std::uint64_t, HunterTask> plans;
  std::map<std::uint64_t, double> arrivals;
  struct Event { double at; std::uint64_t order; std::string id; bool broken; };
  std::vector<Event> events;
  std::map<std::string, InteractionCommand> byId;
  std::set<std::string> acceptedSignals, completedSignals;
  const auto story = s.narrative.CaptureSnapshot();
  for (const auto& [id, signature] : s.receipts) {
    if (signature.rfind("I ", 0) == 0) {
      InteractionCommand a; a.eventId = id; std::uint64_t order; bool accepted;
      if (!detail::ParseInteraction(signature, a, order, accepted) || !InteractionStaticValid(a) ||
          !order || order > serial || a.atSeconds > s.world.NowSeconds() ||
          (a.kind != InteractionOp::Signal && !accepted) || !commands.emplace(order, std::make_pair(a, accepted)).second) return false;
      byId.emplace(id, a); events.push_back({a.atSeconds, order, id, false});
      if (a.kind == InteractionOp::Signal && accepted) acceptedSignals.insert(id);
    } else if (signature.rfind("H ", 0) == 0) {
      HunterCommand a; std::optional<HunterTask> plan;
      if (!detail::ParseHunter(signature, a, plan)) return false;
      if (plan) plans.emplace(plan->serial, *plan);
      if (a.kind == HunterOp::Arrive) arrivals[a.taskSerial] = a.atSeconds;
    }
  }
  if (commands.size() != serial) return false;
  double previousAt = -1;
  for (const auto& [order, entry] : commands) { (void)order; if (entry.first.atSeconds < previousAt) return false; previousAt = entry.first.atSeconds; }
  for (const auto& [id, signature] : s.receipts) if (signature.rfind("H ", 0) == 0) {
    HunterCommand a; std::optional<HunterTask> ignored;
    if (!detail::ParseHunter(signature, a, ignored)) return false;
    const auto plan = plans.find(a.taskSerial);
    if (a.kind == HunterOp::Complete && plan != plans.end() && plan->second.goal == HunterGoal::BreakWedge)
      events.push_back({a.atSeconds, a.taskSerial, id, true});
  }
  std::sort(events.begin(), events.end(), [](const auto& a, const auto& b) { return std::tie(a.at, a.broken, a.order) < std::tie(b.at, b.broken, b.order); });
  for (const auto& e : events) {
    if (e.broken) {
      HunterCommand a; std::optional<HunterTask> ignored;
      if (!detail::ParseHunter(s.receipts.at(e.id), a, ignored)) return false;
      const auto wedge = r.wedges.find(a.target); const auto arrival = arrivals.find(a.taskSerial);
      if (wedge == r.wedges.end() || arrival == arrivals.end() || wedge->second.placedAt > arrival->second) return false;
      r.wedges.erase(wedge); continue;
    }
    const auto& a = byId.at(e.id); const bool accepted = commands.at(e.order).second;
    switch (a.kind) {
      case InteractionOp::Signal:
        if (accepted) {
          if (a.argument == 1) r.quietUntil[a.target] = a.atSeconds + 15;
          else r.orders[a.target] = {static_cast<SignalKind>(a.argument), a.destination, a.eventId, a.atSeconds + 30, false};
        }
        break;
      case InteractionOp::Gesture: {
        const auto key = std::to_string(a.argument); const auto last = r.gestureAt.find(key);
        if (last != r.gestureAt.end() && a.atSeconds < last->second + 30) return false;
        if ((a.argument == 0 && !s.bartholomeu.CaptureSnapshot().appliedEvents.count(a.eventId)) ||
            (a.argument == 2 && a.item == "magazine" && !s.jack.CaptureSnapshot().appliedEvents.count(a.eventId))) return false;
        r.gestureAt[key] = a.atSeconds; break;
      }
      case InteractionOp::PlaceWedge:
        if (!r.wedges.emplace(a.target, InstalledWedge{a.item, a.eventId, a.atSeconds}).second) return false;
        break;
      case InteractionOp::RecoverWedge:
        if (!r.wedges.erase(a.target)) return false;
        break;
      case InteractionOp::Reunion:
        if (!r.reunions.insert(a.target + ":" + a.node).second) return false;
        break;
      case InteractionOp::WitnessFate: {
        const auto& o = story.outcomes.at(a.target);
        const bool real = a.argument == 0 ? o.captured : a.argument == 1 ? o.dead : a.argument == 2 ? o.escapedAlone : o.together;
        if (!real || o.lastContactNode != a.node || !r.knownFates.emplace(a.target, static_cast<WitnessedFate>(a.argument)).second) return false;
        break;
      }
      case InteractionOp::Ambient:
        if (!r.ambientSeen.insert(a.target).second) return false;
        break;
      case InteractionOp::Banter:
        if (!r.banterHeard.insert(a.target).second) return false;
        break;
      case InteractionOp::PerceiveThreat: {
        const auto last = r.threatUntil.find(a.actor);
        if ((last != r.threatUntil.end() && a.atSeconds < last->second) || !s.Person(*detail::Person(a.actor)).CaptureSnapshot().appliedEvents.count(a.eventId)) return false;
        r.threatUntil[a.actor] = a.atSeconds + 30; break;
      }
    }
  }
  for (const auto& [id, signature] : s.receipts) if (signature.rfind("C ", 0) == 0) {
    (void)id; CompanionCommand a;
    if (!detail::ParseCompanion(signature, a)) return false;
    if (a.kind == CompanionOp::Complete && (a.argument == static_cast<int>(companion::Goal::FollowSignal) || a.argument == static_cast<int>(companion::Goal::InspectSignaledArea))) {
      const auto source = byId.find(a.target);
      if (source == byId.end() || !acceptedSignals.count(a.target) || !completedSignals.insert(a.target).second || source->second.kind != InteractionOp::Signal || source->second.target != a.actor || source->second.destination != a.node ||
          a.atSeconds >= source->second.atSeconds + 30 || (source->second.argument == 0) != (a.argument == static_cast<int>(companion::Goal::FollowSignal))) return false;
      const auto order = r.orders.find(a.actor);
      if (order != r.orders.end() && order->second.eventId == a.target) order->second.fulfilled = true;
    }
  }
  for (const auto& door : definition_->doors) {
    const auto* object = s.world.Find(door.id);
    if (!object || (object->stateId == "wedged") != bool(r.wedges.count(door.id)) || (r.wedges.count(door.id) && !object->present)) return false;
  }
  for (const auto& [actor, outcome] : story.outcomes)
    if (outcome.together) r.knownFates[actor] = WitnessedFate::Together;
  return true;
}
bool CampaignCore::InteractionCoherent(const State& s) const {
  State restored(s);
  return RestoreInteractions(restored) && Fingerprint(s.interactions) == Fingerprint(restored.interactions);
}
} // namespace thug::campaign
