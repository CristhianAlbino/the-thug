#include "CampaignState.h"
#include "InteractionIO.h"
#include "CompanionIO.h"
#include "HunterIO.h"
#include <algorithm>
#include <limits>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& entries, const std::string& id) {
  for (const auto& entry : entries) if (entry.id == id) return &entry;
  return nullptr;
}
bool At(const InteractionContext& c, const std::string& actor, const std::string& node, bool active = true) {
  const auto it = c.actors.find(actor);
  return it != c.actors.end() && it->second.node == node && (!active || it->second.active);
}
bool Available(const save::NarrativeSnapshot& story, const std::string& actor) {
  if (actor == "joe") return true;
  const auto& o = story.outcomes.at(actor);
  return !o.dead && !o.captured && !o.escapedAlone && !o.together;
}
std::string Name(const std::string& actor) {
  if (actor == "bartholomeu") return "Bartholomeu";
  if (actor == "harley") return "Harley";
  if (actor == "jack") return "Jack";
  return "Joe";
}
}
InteractionRuntimeSnapshot CampaignCore::Interactions() const { return state_->interactions; }
std::optional<NightVariation> CampaignCore::VariationFor(const std::string& id) const {
  const auto* v = Find(definition_->variations, id);
  if (!v) return {};
  const auto index = state_->interactions.variation;
  return NightVariation{index, v->id, v->node, v->ambient.at(index), v->speaker, v->lines.at(index)};
}
std::optional<director::Cue> CampaignCore::InteractionCue(const InteractionCommand& a) const {
  if (!InteractionStaticValid(a)) return {};
  director::Cue cue; cue.owner = "interaction";
  if (a.kind == InteractionOp::Banter) {
    const auto v = VariationFor(a.target);
    cue.id = a.target + ":" + std::to_string(v->variant); cue.speaker = v->speaker; cue.text = v->line;
    return cue;
  }
  if (a.kind != InteractionOp::Gesture) return {};
  cue.id = "IG_" + std::to_string(a.argument); cue.kind = director::CueKind::Gesture; cue.speaker = a.actor;
  cue.text = a.argument == 0 ? "Harley abaixa a voz, segura o antebraço de Bartholomeu e espera que ele firme a respiração." :
    a.argument == 1 ? (state_->bartholomeu.CurrentTraits().fear >= 20 ? "Bartholomeu confere o corredor com a mão trêmula e termina de tratar Joe." : "Bartholomeu confere o corredor, ajeita o tratamento e ajuda Joe.") :
    "Jack passa o recurso para Joe, dá um sorriso curto e volta a olhar a porta.";
  cue.asset = cue.id; return cue;
}
bool CampaignCore::InteractionStaticValid(const InteractionCommand& a) const {
  if (!detail::Token(a.eventId) || !detail::Token(a.target) || !definition_->nodes.count(a.node) ||
      !std::isfinite(a.atSeconds) || a.atSeconds < 0 || a.kind < InteractionOp::Signal || a.kind > InteractionOp::PerceiveThreat ||
      (a.actor != "joe" && !detail::Person(a.actor))) return false;
  if (a.kind != InteractionOp::Signal && !a.destination.empty()) return false;
  if (a.kind != InteractionOp::Gesture && a.kind != InteractionOp::PlaceWedge && a.kind != InteractionOp::RecoverWedge && !a.item.empty()) return false;
  if (a.kind != InteractionOp::Signal && a.kind != InteractionOp::Gesture && a.kind != InteractionOp::WitnessFate && a.argument) return false;
  switch (a.kind) {
    case InteractionOp::Signal:
      return a.actor == "joe" && detail::Person(a.target) && a.argument >= 0 && a.argument <= 2 &&
        (a.argument == 1 ? a.destination.empty() : definition_->nodes.count(a.destination) && (a.argument != 0 || a.destination == a.node));
    case InteractionOp::Gesture:
      return (a.argument == 0 && a.actor == "harley" && a.target == "bartholomeu" && a.item.empty()) ||
        (a.argument == 1 && a.actor == "bartholomeu" && a.target == "joe" && a.item == "treatment") ||
        (a.argument == 2 && a.actor == "jack" && a.target == "joe" && (a.item == "magazine" || a.item == "window_tool" || a.item == "smoke"));
    case InteractionOp::PlaceWedge: case InteractionOp::RecoverWedge: {
      const auto* w = Find(definition_->wedges, a.target);
      const auto* d = Find(definition_->doors, a.target);
      return a.actor == "joe" && w && d && a.item == w->item && (a.node == d->fromNode || a.node == d->toNode);
    }
    case InteractionOp::Reunion: return a.actor == "joe" && bool(detail::Person(a.target));
    case InteractionOp::WitnessFate: return a.actor == "joe" && detail::Person(a.target) && a.argument >= 0 && a.argument <= 3;
    case InteractionOp::Ambient: case InteractionOp::Banter: {
      const auto* v = Find(definition_->variations, a.target);
      return a.actor == "joe" && v && a.node == v->node;
    }
    case InteractionOp::PerceiveThreat: return detail::Person(a.actor) && a.target == "hunter";
  }
  return false;
}
InteractionResult CampaignCore::ApplyInteraction(const InteractionCommand& a, const InteractionContext& c) {
  if (!InteractionStaticValid(a)) return {Status::InvalidAction};
  const auto found = state_->receipts.find(a.eventId);
  if (found != state_->receipts.end()) {
    InteractionCommand saved; std::uint64_t order; bool accepted;
    return {detail::ParseInteraction(found->second, saved, order, accepted) &&
      detail::Signature(saved, order, accepted) == detail::Signature(a, order, accepted) ? Status::AlreadyApplied : Status::Conflict};
  }
  if (Paused()) return {Status::Paused};
  if (Ending() != rules::Ending::None && a.kind != InteractionOp::WitnessFate) return {Status::Finished};
  if (state_->special.phase == SpecialPhase::Collapsing ||
      (state_->special.phase == SpecialPhase::Confronting && Now() >= state_->special.boundaryAt)) return {Status::AwaitingPhysics};
  if (a.atSeconds != Now()) return {Status::InvalidTime};
  if (!a.physicallyVerified || !c.actionCompleted || c.actors.size() > 4 || !At(c, a.actor, a.node) ||
      !std::isfinite(c.actualSourceMeters.x) || !std::isfinite(c.actualSourceMeters.y) || !std::isfinite(c.actualSourceMeters.z)) return {Status::InvalidAction};
  for (const auto& [name, actor] : c.actors) {
    if ((name != "joe" && !detail::Person(name)) || !definition_->nodes.count(actor.node)) return {Status::InvalidAction};
    const auto& hide = name == "joe" ? state_->hidingPlace : state_->bodies.at(name).hidingPlace;
    if (!hide.empty() && actor.node != Find(definition_->hides, hide)->atNode) return {Status::MissingRequirement};
  }
  for (const auto& node : c.reachableNodes) if (!definition_->nodes.count(node)) return {Status::InvalidAction};
  if (state_->receipts.size() + detail::CombatReservedReceipts(state_->combat) + detail::SpecialReservedReceipts(state_->special) >= detail::MaxReceipts ||
      state_->interactions.serial == std::numeric_limits<std::uint64_t>::max()) return {Status::LogFull};
  auto next = std::make_unique<State>(*state_);
  const auto result = ExecuteInteraction(*next, a, c);
  if (result.status != Status::Applied) return result;
  next->SyncEnding(); ReconcileCombat(*next);
  next->receipts.emplace(a.eventId, detail::Signature(a, ++next->interactions.serial, result.accepted));
  state_.swap(next); return result;
}
InteractionResult CampaignCore::ExecuteInteraction(State& s, const InteractionCommand& a, const InteractionContext& c) {
  const InteractionResult denied{Status::MissingRequirement};
  const auto story = s.narrative.CaptureSnapshot();
  if (!Available(story, a.actor)) return denied;
  if (a.actor == "joe") {
    if (s.survival.CurrentEffects().collapsed || s.survival.CurrentEffects().dead) return denied;
    if (!s.hidingPlace.empty()) {
      const auto* hide = Find(definition_->hides, s.hidingPlace);
      if (a.node != hide->atNode || (a.kind != InteractionOp::Signal && a.kind != InteractionOp::WitnessFate)) return denied;
    }
  } else {
    const auto& b = s.bodies.at(a.actor);
    if (b.survival.CurrentEffects().collapsed || b.survival.CurrentEffects().dead ||
        (!b.hidingPlace.empty() && a.node != Find(definition_->hides, b.hidingPlace)->atNode) ||
        (a.kind == InteractionOp::Gesture && (b.task || !b.hidingPlace.empty()))) return denied;
  }
  auto& r = s.interactions;
  switch (a.kind) {
    case InteractionOp::Signal: {
      const auto peer = c.actors.find(a.target);
      if (!c.signalPerceived || peer == c.actors.end() || !peer->second.active || !Available(story, a.target)) return denied;
      if (a.argument == 2 && !c.areaPerceived) return denied;
      const auto& b = s.bodies.at(a.target);
      if (!b.hidingPlace.empty() && peer->second.node != Find(definition_->hides, b.hidingPlace)->atNode) return denied;
      const auto& t = s.Person(*detail::Person(a.target)).CurrentTraits();
      const bool accepted = !b.task && !b.survival.CurrentEffects().collapsed &&
        t.trust >= (c.inDanger ? 55 : 35) && (a.argument == 1 || (b.hidingPlace.empty() && t.fear + t.stress < 175));
      if (a.argument != 1 && !c.reachableNodes.count(a.destination)) return denied;
      if (accepted) {
        if (a.argument == 1) r.quietUntil[a.target] = a.atSeconds + 15;
        else r.orders[a.target] = {static_cast<SignalKind>(a.argument), a.destination, a.eventId, a.atSeconds + 30, false};
      }
      return {Status::Applied, accepted};
    }
    case InteractionOp::Gesture: {
      if (!At(c, "joe", a.node) || s.survival.CurrentEffects().collapsed || s.survival.CurrentEffects().dead ||
          !At(c, a.target, a.node) || !Available(story, a.target) || c.inDanger) return denied;
      const auto key = std::to_string(a.argument);
      const auto previous = r.gestureAt.find(key);
      if (previous != r.gestureAt.end() && a.atSeconds < previous->second + 30) return denied;
      if (a.argument == 0) {
        const auto& target = s.bodies.at(a.target);
        if (target.survival.CurrentEffects().collapsed || !target.hidingPlace.empty() || target.task) return denied;
        companion::Influence delta{}; delta.fear = -12; delta.stress = -15;
        if (!s.bartholomeu.ApplyInfluence(delta, a.eventId, a.atSeconds)) return denied;
      } else {
        auto& source = s.bodies.at(a.actor);
        const auto& traits = s.Person(*detail::Person(a.actor)).CurrentTraits();
        if (traits.trust < 40 || source.survival.CurrentInjury() > survival::Injury::Light || traits.fear >= 95) return denied;
        if (a.argument == 1) {
          if (!source.inventory.Remove(a.item, 1, survival::RemovalReason::Consume) || !s.survival.Treat()) return denied;
        } else {
          if (!source.inventory.TransferTo(s.inventory, a.item, 1)) return denied;
          if (a.item == "magazine") { companion::Influence delta{}; delta.ammunition = -1;
            if (!s.jack.ApplyInfluence(delta, a.eventId, a.atSeconds)) return denied; }
        }
      }
      r.gestureAt[key] = a.atSeconds; break;
    }
    case InteractionOp::PlaceWedge: case InteractionOp::RecoverWedge: {
      auto object = *s.world.Find(a.target);
      if (!object.present) return denied;
      if (a.kind == InteractionOp::PlaceWedge) {
        // A new installation starts on a later simulation instant than a
        // completed Hunter interaction on this door; ledger causality is clear.
        for (const auto& [id, signature] : s.receipts) if (signature.rfind("H ", 0) == 0) {
          (void)id; HunterCommand old; std::optional<HunterTask> plan;
          if (detail::ParseHunter(signature, old, plan) && old.kind == HunterOp::Complete && old.target == a.target && old.atSeconds == a.atSeconds) return denied;
        }
        if (!c.alternativePathVerified || object.stateId != "closed" || r.wedges.count(a.target) ||
            !CombatItemAvailable(s, a.item, 1) || !s.inventory.Remove(a.item, 1, survival::RemovalReason::Consume)) return denied;
        object.stateId = "wedged"; r.wedges.emplace(a.target, InstalledWedge{a.item, a.eventId, a.atSeconds});
      } else {
        const auto wedge = r.wedges.find(a.target);
        if (object.stateId != "wedged" || wedge == r.wedges.end() || wedge->second.item != a.item || !s.inventory.Add(a.item, 1)) return denied;
        object.stateId = "closed"; r.wedges.erase(wedge);
      }
      if (!s.world.SetObject(a.target, object, a.atSeconds)) return denied;
      return {Status::Applied, true, world::WorldSignal{a.target, perception::SoundKind::Door, object.positionMeters, a.atSeconds, .15}};
    }
    case InteractionOp::Reunion: {
      if (!At(c, a.target, a.node) || !Available(story, a.target)) return denied;
      const auto& body = s.bodies.at(a.target);
      if (!body.hidingPlace.empty() && a.node != Find(definition_->hides, body.hidingPlace)->atNode) return denied;
      if (!r.reunions.insert(a.target + ":" + a.node).second) return denied;
      break;
    }
    case InteractionOp::WitnessFate: {
      if (!At(c, a.target, a.node, false)) return denied;
      const auto& fate = story.outcomes.at(a.target);
      const bool real = a.argument == 0 ? fate.captured : a.argument == 1 ? fate.dead : a.argument == 2 ? fate.escapedAlone : fate.together;
      if (!real || fate.lastContactNode != a.node || r.knownFates.count(a.target)) return denied;
      r.knownFates[a.target] = static_cast<WitnessedFate>(a.argument); break;
    }
    case InteractionOp::Ambient: case InteractionOp::Banter: {
      const auto* v = Find(definition_->variations, a.target);
      auto& delivered = a.kind == InteractionOp::Ambient ? r.ambientSeen : r.banterHeard;
      if (a.kind == InteractionOp::Banter) {
        const auto quiet = r.quietUntil.find(v->speaker);
        const auto& body = s.bodies.at(v->speaker);
        if (c.inDanger || !At(c, v->speaker, a.node) || !Available(story, v->speaker) ||
            (!body.hidingPlace.empty() && Find(definition_->hides, body.hidingPlace)->atNode != a.node) ||
            (quiet != r.quietUntil.end() && quiet->second > a.atSeconds) || !story.activeScene.empty() || !story.activeBanter.empty()) return denied;
      }
      if (!delivered.insert(a.target).second) return denied;
      if (a.kind == InteractionOp::Ambient)
        return {Status::Applied, true, world::WorldSignal{a.target, perception::SoundKind::Distraction, c.actualSourceMeters, a.atSeconds, .25}};
      break;
    }
    case InteractionOp::PerceiveThreat: {
      const auto last = r.threatUntil.find(a.actor);
      if (!c.hunterPerceived || (last != r.threatUntil.end() && a.atSeconds < last->second)) return denied;
      companion::Influence delta{}; delta.fear = 10; delta.stress = 12;
      if (!s.Person(*detail::Person(a.actor)).ApplyInfluence(delta, a.eventId, a.atSeconds)) return denied;
      r.threatUntil[a.actor] = a.atSeconds + 30; break;
    }
  }
  if (a.actor != "joe" && !s.bodies.at(a.actor).task) s.bodies.at(a.actor).lastNode = a.node;
  return {Status::Applied, true};
}
std::map<std::string, std::string> CampaignCore::KnownCompanionFates() const {
  std::map<std::string, std::string> fates;
  for (const auto& name : {"harley", "bartholomeu", "jack"}) {
    const auto known = state_->interactions.knownFates.find(name);
    fates[name] = known == state_->interactions.knownFates.end() ? "paradeiro desconhecido" :
      known->second == WitnessedFate::Captured ? "captura testemunhada" : known->second == WitnessedFate::Dead ? "morte testemunhada" :
      known->second == WitnessedFate::LeftAlone ? "partiu sozinho; destino não confirmado" : "saiu comigo";
  }
  return fates;
}
std::vector<NightRecordEntry> CampaignCore::NightRecord() const {
  std::vector<NightRecordEntry> entries;
  for (const auto& [id, signature] : state_->receipts) {
    std::string text, actor = "joe", node; double at = 0;
    if (signature.rfind("I ", 0) == 0) {
      InteractionCommand a; std::uint64_t order; bool accepted;
      if (!detail::ParseInteraction(signature, a, order, accepted) || a.kind == InteractionOp::PerceiveThreat) continue;
      actor = a.actor; node = a.node; at = a.atSeconds;
      switch (a.kind) {
        case InteractionOp::Signal: text = "Sinalizei para " + Name(a.target) + (accepted ? "; aceitou." : "; recusou."); break;
        case InteractionOp::Gesture: text = a.argument == 0 ? "Harley ajudou Bartholomeu a se acalmar." : a.argument == 1 ? "Bartholomeu cuidou do meu ferimento." : "Jack me passou um recurso."; break;
        case InteractionOp::PlaceWedge: text = "Deixei uma ferramenta segurando a porta."; break;
        case InteractionOp::RecoverWedge: text = "Voltei para recuperar a ferramenta da porta."; break;
        case InteractionOp::Reunion: text = "Encontrei " + Name(a.target) + " aqui."; break;
        case InteractionOp::WitnessFate: text = a.argument == 0 ? "Vi " + Name(a.target) + " ser capturado." : a.argument == 1 ? "Confirmei a morte de " + Name(a.target) + "." : a.argument == 2 ? "Vi " + Name(a.target) + " partir sozinho." : Name(a.target) + " saiu comigo."; break;
        case InteractionOp::Ambient: text = "Percebi uma ocorrência no hospital: " + VariationFor(a.target)->ambient; break;
        case InteractionOp::Banter: text = "Ouvi " + Name(VariationFor(a.target)->speaker) + ": " + VariationFor(a.target)->line; break;
        default: break;
      }
    } else if (signature.rfind("C ", 0) == 0) {
      CompanionCommand a; if (!detail::ParseCompanion(signature, a)) continue;
      actor = a.actor; node = a.node; at = a.atSeconds;
      if (a.actor == "joe" && a.kind == CompanionOp::Abandon) text = "Deixei " + Name(a.target) + " seguir sem mim.";
      if (a.actor == "joe" && a.kind == CompanionOp::Transfer) text = "Dividi um recurso com " + Name(a.target) + ".";
      if ((a.kind == CompanionOp::Treat || a.kind == CompanionOp::Stabilize) && (a.actor == "joe" || a.target == "joe")) text = a.actor == "joe" ? "Ajudei " + Name(a.target) + " com um tratamento." : Name(a.actor) + " cuidou do meu ferimento.";
      if (a.kind == CompanionOp::Complete && a.argument == static_cast<int>(companion::Goal::HelpJoe)) text = Name(a.actor) + " cuidou do meu ferimento.";
      if (a.kind == CompanionOp::Complete && a.argument == static_cast<int>(companion::Goal::ShareRoute)) text = Name(a.actor) + " me contou sobre uma rota que conhecia.";
      if (a.kind == CompanionOp::Complete && a.argument == static_cast<int>(companion::Goal::FollowSignal)) text = Name(a.actor) + " veio até mim.";
      if (a.kind == CompanionOp::Outcome && a.argument == static_cast<int>(CompanionOutcome::Together)) text = Name(a.actor) + " saiu comigo.";
    } else {
      Action a; if (!detail::ParseSignature(signature, a)) continue;
      node = a.node; at = a.atSeconds;
      if (a.kind == ActionKind::InspectDocument) text = "Examinei um documento.";
      if (a.kind == ActionKind::InspectAudio) text = "Ouvi uma gravação.";
      if (a.kind == ActionKind::ShareAmmo) text = "Dividi munição com " + Name(a.target) + ".";
    }
    if (!text.empty()) entries.push_back({id, actor, node, text, at});
  }
  std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return std::tie(a.atSeconds, a.eventId) < std::tie(b.atSeconds, b.eventId); });
  return entries;
}
} // namespace thug::campaign
