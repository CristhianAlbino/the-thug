#include "CampaignState.h"
#include "SpecialIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
bool Tag(std::istream& in, const char* expected) { std::string tag; return bool(in >> tag) && tag == expected; }
void Set(std::ostream& out, const char* tag, const std::set<std::string>& values) {
  out << tag << ' ' << values.size(); for (const auto& value : values) out << ' ' << std::quoted(value); out << '\n';
}
bool Set(std::istream& in, const char* tag, std::set<std::string>& values, std::size_t limit) {
  std::size_t count; if (!Tag(in, tag) || !(in >> count) || count > limit) return false;
  for (std::size_t i = 0; i < count; ++i) { std::string value;
    if (!(in >> std::quoted(value)) || !detail::Token(value) || !values.insert(value).second) return false;
  } return true;
}
void Write(std::ostream& out, const SpecialRuntimeSnapshot& r) {
  out << std::setprecision(std::numeric_limits<double>::max_digits10);
  out << "SPECIAL " << r.commandSerial << ' ' << r.plantSerial << ' ' << r.appearanceTicket << ' '
      << static_cast<int>(r.phase) << ' ' << r.boundaryAt << '\n';
  Set(out, "RECRUITED", r.recruited); Set(out, "SPECIAL_SUPPLIES", r.supplies);
  Set(out, "PLANTED", r.planted); Set(out, "APPEARANCES", r.appearances);
  out << "PLANT " << bool(r.plant) << '\n';
  if (r.plant) { const auto& p = *r.plant;
    out << p.serial << ' ' << std::quoted(p.site) << ' ' << std::quoted(p.eventId) << ' ' << p.startedAt << ' ' << p.readyAt << '\n';
  }
}
}
void CampaignCore::WriteSpecial(std::ostream& out, const State& s) const { Write(out, s.special); }
bool CampaignCore::ReadSpecial(std::istream& in, State& s) const {
  SpecialRuntimeSnapshot r; int phase, plant;
  if (!Tag(in, "SPECIAL") || !(in >> r.commandSerial >> r.plantSerial >> r.appearanceTicket >> phase >> r.boundaryAt) ||
      phase < 0 || phase > 7 || r.appearanceTicket >= 1000 ||
      !Set(in, "RECRUITED", r.recruited, 3) || !Set(in, "SPECIAL_SUPPLIES", r.supplies, 32) ||
      !Set(in, "PLANTED", r.planted, 16) || !Set(in, "APPEARANCES", r.appearances, 16) ||
      !Tag(in, "PLANT") || !(in >> plant) || plant < 0 || plant > 1) return false;
  r.phase = static_cast<SpecialPhase>(phase);
  if (plant) { PlantTask p;
    if (!(in >> p.serial >> std::quoted(p.site) >> std::quoted(p.eventId) >> p.startedAt >> p.readyAt) ||
        !detail::Token(p.site) || !detail::Token(p.eventId)) return false;
    r.plant = p;
  }
  s.special = std::move(r); return true;
}

bool CampaignCore::SpecialCoherent(const State& s) const {
  const auto& actual = s.special;
  const auto story = s.narrative.CaptureSnapshot();
  const auto mode = s.routes.DifficultyLevel(); const double now = s.world.NowSeconds();
  // Appearance knowledge is projected from this module, never owned by the
  // free narrative fact set (including after loading a checksum-valid file).
  for (const auto& fact : story.durableFacts) if (fact.rfind("appearance:", 0) == 0) return false;
  if (actual.appearanceTicket >= 1000 || !std::isfinite(actual.boundaryAt) ||
      s.receipts.size() + detail::CombatReservedReceipts(s.combat) + detail::SpecialReservedReceipts(actual) + detail::RescueReservedReceipts(s.rescue) > detail::MaxReceipts) return false;
  std::map<std::uint64_t, SpecialCommand> commands;
  for (const auto& [id, signature] : s.receipts) if (signature.rfind("S ", 0) == 0) {
    SpecialCommand c; c.eventId = id; std::uint64_t order;
    if (!detail::ParseSpecial(signature, c, order) || !SpecialStaticValid(c) || c.atSeconds > now ||
        !order || !commands.emplace(order, c).second) return false;
  }
  SpecialRuntimeSnapshot r; r.appearanceTicket = actual.appearanceTicket;
  double previous = 0;
  for (const auto& [order, c] : commands) {
    if (order != ++r.commandSerial || c.atSeconds < previous) return false;
    previous = c.atSeconds;
    if (r.phase == SpecialPhase::Opportunity && c.atSeconds >= r.boundaryAt) { r.phase = SpecialPhase::Failed; r.boundaryAt = -1; }
    if (r.phase == SpecialPhase::Collapsing && c.kind != SpecialOp::ResolveCollapse) return false;
    if (r.phase == SpecialPhase::Escaped || r.phase == SpecialPhase::Destroyed || r.phase == SpecialPhase::FatalCollapse) return false;
    if (r.phase == SpecialPhase::Confronting && c.atSeconds >= r.boundaryAt && c.kind != SpecialOp::ResolveConfrontation) return false;
    if (c.kind == SpecialOp::WitnessAppearance) {
      const auto at = std::find_if(definition_->appearances.begin(), definition_->appearances.end(), [&](const auto& a) { return a.id == c.target; });
      if (at == definition_->appearances.end() || at->node != c.node || !(at->modes & (1u << static_cast<int>(mode))) ||
          story.act < at->minimumAct || r.appearanceTicket >= at->tickets || !r.appearances.insert(c.target).second) return false;
      continue;
    }
    if (!definition_->finale || story.act != 5 || mode != (c.kind <= SpecialOp::ConfrontationEscape ? rules::Difficulty::Easy : rules::Difficulty::Punitive)) return false;
    const auto& d = *definition_->finale;
    switch (c.kind) {
      case SpecialOp::Recruit:
        if (r.phase != SpecialPhase::Idle || !r.recruited.insert(c.target).second) return false;
        break;
      case SpecialOp::StartConfrontation:
        if (r.phase != SpecialPhase::Idle || r.recruited.size() < static_cast<std::size_t>(d.minimumCompanions)) return false;
        r.phase = SpecialPhase::Confronting; r.boundaryAt = c.atSeconds + d.confrontationSeconds;
        break;
      case SpecialOp::ResolveConfrontation:
        if (r.phase != SpecialPhase::Confronting || c.atSeconds != r.boundaryAt) return false;
        for (const auto& [actor, fate] : c.fates) { (void)fate; if (!r.recruited.count(actor)) return false; }
        r.phase = c.argument ? SpecialPhase::Opportunity : SpecialPhase::Failed;
        r.boundaryAt = c.argument ? c.atSeconds + d.opportunitySeconds : -1;
        break;
      case SpecialOp::ConfrontationEscape:
        if (r.phase != SpecialPhase::Opportunity || c.atSeconds >= r.boundaryAt) return false;
        for (const auto& [actor, fate] : c.fates) { (void)fate; if (!r.recruited.count(actor)) return false; }
        r.phase = SpecialPhase::Escaped; r.boundaryAt = -1;
        break;
      case SpecialOp::AcquireSupply:
        if (!r.supplies.insert(c.target).second) return false;
        break;
      case SpecialOp::BeginPlant:
        if (r.phase != SpecialPhase::Idle || r.plant || r.planted.count(c.target)) return false;
        r.plant = PlantTask{++r.plantSerial, c.target, c.eventId, c.atSeconds, c.atSeconds + d.plantSeconds};
        break;
      case SpecialOp::CompletePlant: case SpecialOp::CancelPlant:
        if (!r.plant || r.plant->serial != c.serial || r.plant->site != c.target) return false;
        if (c.kind == SpecialOp::CompletePlant && (c.atSeconds < r.plant->readyAt || !r.planted.insert(c.target).second)) return false;
        r.plant.reset(); break;
      case SpecialOp::DetonateCollapse:
        if (r.phase != SpecialPhase::Idle || r.plant || r.planted.size() != d.sites.size()) return false;
        r.phase = SpecialPhase::Collapsing; r.boundaryAt = c.atSeconds + d.collapseSeconds; break;
      case SpecialOp::ResolveCollapse:
        if (r.phase != SpecialPhase::Collapsing || c.atSeconds != r.boundaryAt) return false;
        r.phase = c.argument ? SpecialPhase::Destroyed : SpecialPhase::FatalCollapse; r.boundaryAt = -1; break;
      default: return false;
    }
  }
  const auto ending = s.routes.Result();
  if (r.phase == SpecialPhase::Opportunity && now >= r.boundaryAt) { r.phase = SpecialPhase::Failed; r.boundaryAt = -1; }
  const bool finished = ending != rules::Ending::None;
  if (finished) {
    r.plant.reset();
    if (r.phase == SpecialPhase::Confronting || r.phase == SpecialPhase::Opportunity || r.phase == SpecialPhase::Collapsing) { r.phase = SpecialPhase::Failed; r.boundaryAt = -1; }
  }
  if (!finished && (r.phase == SpecialPhase::Confronting || r.phase == SpecialPhase::Collapsing) && now > r.boundaryAt) return false;
  if (actual.plant && s.inventory.Quantity("structural_charge") < 1) return false;
  if (bool(story.durableFacts.count("rare_confrontation")) != (r.phase == SpecialPhase::Escaped) ||
      bool(story.durableFacts.count("rare_destruction")) != (r.phase == SpecialPhase::Destroyed) ||
      (r.phase == SpecialPhase::Escaped) != (ending == rules::Ending::CoordinatedEscape) ||
      (r.phase == SpecialPhase::Destroyed) != (ending == rules::Ending::BitterDestruction)) return false;
  if (r.phase == SpecialPhase::Destroyed && (s.survival.CurrentInjury() != survival::Injury::Severe || s.survival.IsBleeding() || story.joeEscaped)) return false;
  if (r.phase == SpecialPhase::FatalCollapse && (ending != rules::Ending::Died || !s.survival.CurrentEffects().dead)) return false;
  if (definition_->finale) {
    for (const auto& supply : definition_->finale->supplies) {
      const auto* object = s.world.Find(supply.id); const bool collected = r.supplies.count(supply.id);
      if (!object || object->present == collected || object->stateId != (collected ? "collected" : "available")) return false;
    }
    for (const auto& site : definition_->finale->sites) {
      const auto* object = s.world.Find(site.id);
      const auto expected = r.phase == SpecialPhase::Destroyed || r.phase == SpecialPhase::FatalCollapse ? "destroyed" : r.planted.count(site.id) ? "planted" : "empty";
      if (!object || !object->present || object->stateId != expected) return false;
    }
  }
  const auto ownedPresentation = [&](const std::string& id) {
    return id.rfind("SP_", 0) != 0 || std::any_of(definition_->appearances.begin(), definition_->appearances.end(),
        [&](const auto& a) { return a.block == id && r.appearances.count(a.id); });
  };
  if (!ownedPresentation(story.activeScene)) return false;
  for (const auto& id : story.seenScenes) if (!ownedPresentation(id)) return false;
  std::ostringstream expected, received; Write(expected, r); Write(received, actual);
  return expected.str() == received.str();
}
} // namespace thug::campaign
