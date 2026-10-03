#include "CampaignCore.h"
#include "CampaignData.generated.h"
#include "InteractionIO.h"
#include "CompanionIO.h"
#include <iostream>
#include <stdexcept>
using namespace thug;
using namespace thug::campaign;
namespace {
int checks = 0, sequence = 0;
void Check(bool ok, const char* why) { ++checks; if (!ok) throw std::runtime_error(why); }
std::string Id() { return "interaction-test-" + std::to_string(++sequence); }
std::string Bytes(const CampaignCore& c) { std::ostringstream out; Check(c.Save(out) == save::Result::Ok, "Interaction checkpoint coherent"); return out.str(); }
void Load(CampaignCore& c, const std::string& bytes) { std::istringstream in(bytes); Check(c.Load(in) == save::Result::Ok, "Interaction checkpoint restores"); Check(Bytes(c) == bytes, "Load preserves all clocks, resources, choices and knowledge"); }
InteractionContext Context(const std::string& node) {
  InteractionContext c; c.actionCompleted = c.signalPerceived = c.areaPerceived = c.alternativePathVerified = true;
  for (const auto& name : {"joe", "harley", "bartholomeu", "jack"}) c.actors.emplace(name, director::Actor{node, true});
  c.reachableNodes.insert(node); return c;
}
InteractionCommand Cmd(const CampaignCore& c, InteractionOp kind, const std::string& target, const std::string& node, int argument = 0) {
  InteractionCommand a; a.eventId = Id(); a.kind = kind; a.target = target; a.node = node; a.atSeconds = c.Now(); a.argument = argument; a.physicallyVerified = true; return a;
}
void Accepted(CampaignCore& c, const InteractionCommand& a, const InteractionContext& ctx, bool accepted = true) {
  const auto result = c.ApplyInteraction(a, ctx); Check(result.status == Status::Applied && result.accepted == accepted, "Physical interaction commits expected response"); Bytes(c);
}
void Reject(CampaignCore& c, const InteractionCommand& a, const InteractionContext& ctx, Status status = Status::MissingRequirement) {
  const auto before = Bytes(c); Check(c.ApplyInteraction(a, ctx).status == status, "Invalid interaction rejected"); Check(Bytes(c) == before, "Rejection changes no resources, pressure, randomness or journal");
}
void Tick(CampaignCore& c, double dt) { Check(c.Tick(dt, survival::Activity::Rest).status == Status::Applied, "Shared time advances"); }
void Joe(CampaignCore& c, ActionKind kind, const std::string& target, const std::string& node, int arg = 0) {
  Check(c.Apply({Id(), kind, target, node, c.Now(), arg, true}).status == Status::Applied, "Joe fixture action commits"); Bytes(c);
}
CompanionContext Party(const std::string& node) {
  CompanionContext c; c.actionCompleted = c.movementCompleted = true; c.reachableNodes.insert(node);
  for (const auto& name : {"joe", "harley", "bartholomeu", "jack"}) c.actors.emplace(name, director::Actor{node,true});
  return c;
}
void Share(CampaignCore& c, const std::string& target, const std::string& item, const std::string& node) {
  CompanionCommand a{Id(), CompanionOp::Transfer, "joe", target, node, c.Now()}; a.item = item; a.quantity = 1; a.physicallyVerified = true;
  Check(c.ApplyCompanion(a, Party(node)).status == Status::Applied, "Own resource transferred"); Bytes(c);
}
bool Has(const std::vector<CompanionOffer>& offers, companion::Goal goal) { for (const auto& o : offers) if (o.goal == goal) return true; return false; }
std::string Checksum(std::string bytes) {
  const auto at = bytes.rfind("CHECKSUM "); bytes.resize(at); return bytes + "CHECKSUM " + detail::Hash(bytes) + '\n';
}
void SignalsAndMovement() {
  for (const auto mode : {rules::Difficulty::Easy, rules::Difficulty::Normal, rules::Difficulty::Hard, rules::Difficulty::Punitive}) {
    CampaignCore c(mode, 71); auto ctx = Context("F4_START");
    auto a = Cmd(c, InteractionOp::Signal, "harley", "F4_START"); a.destination = a.node;
    auto unseen = ctx; unseen.signalPerceived = false; Reject(c, a, unseen);
    auto ui = a; ui.physicallyVerified = false; Reject(c, ui, ctx, Status::InvalidAction);
    Accepted(c, a, ctx); const auto before = Bytes(c);
    Check(c.ApplyInteraction(a, ctx).status == Status::AlreadyApplied && Bytes(c) == before, "Signal ACK cannot repeat");
    auto conflicting = a; conflicting.argument = 1; conflicting.destination.clear(); Check(c.ApplyInteraction(conflicting, ctx).status == Status::Conflict, "Same signal ID cannot change intent");
    auto party = Party("F4_START"); Check(Has(c.CompanionOffers("harley", "F4_START", party), companion::Goal::FollowSignal), "Perceived call creates executable choice");
    CompanionCommand begin{Id(), CompanionOp::BeginTask, "harley", a.eventId, "F4_START", c.Now()}; begin.argument = static_cast<int>(companion::Goal::FollowSignal); begin.physicallyVerified = true;
    Check(c.ApplyCompanion(begin, party).status == Status::Applied, "Companion starts travel, not teleport");
    CampaignCore restored(mode, 900); Load(restored, Bytes(c));
    auto forged = Bytes(c); const auto response = forged.find("I 1 1 0"); Check(response != std::string::npos, "Fixture has accepted call receipt"); forged[response + 4] = '0';
    const auto original = Bytes(restored); std::istringstream altered(Checksum(forged));
    Check(restored.Load(altered) == save::Result::IncompatibleRoster && Bytes(restored) == original, "Travelling plan cannot survive a forged refusal even with valid checksum");
    auto task = restored.CompanionRuntime(companion::Character::Harley).task;
    Check(task && task->phase == TaskPhase::Travelling, "Travelling signal plan persists");
    CompanionCommand arrive{Id(), CompanionOp::Arrive, "harley", a.eventId, "F4_START", restored.Now()}; arrive.taskSerial = task->serial; arrive.physicallyVerified = true;
    Check(restored.ApplyCompanion(arrive, party).status == Status::Applied, "Actual arrival confirms travel"); Tick(restored, 1);
    auto complete = arrive; complete.eventId = Id(); complete.kind = CompanionOp::Complete; complete.atSeconds = restored.Now(); complete.argument = begin.argument;
    auto departed = party; departed.actors.erase("joe"); const auto safe = Bytes(restored);
    Check(restored.ApplyCompanion(complete, departed).status == Status::MissingRequirement && Bytes(restored) == safe, "Calling companion cannot reunite with absent Joe");
    Check(restored.ApplyCompanion(complete, party).status == Status::Applied, "Actual reunion finishes order");
    Check(restored.Interactions().orders.at("harley").fulfilled && !Has(restored.CompanionOffers("harley", "F4_START", party), companion::Goal::FollowSignal), "Completed signal offers no repeating task"); Load(restored, Bytes(restored));
    auto point = Cmd(restored, InteractionOp::Signal, "harley", "F4_START", 2); point.destination = "F2_SUPPLY"; ctx.reachableNodes.insert(point.destination);
    auto unknownArea = ctx; unknownArea.areaPerceived = false; Reject(restored, point, unknownArea);
    Accepted(restored, point, ctx); party.reachableNodes.insert(point.destination);
    Check(Has(restored.CompanionOffers("harley", "F4_START", party), companion::Goal::InspectSignaledArea), "Pointed area is a choice to inspect");
    Check(restored.CompanionRuntime(companion::Character::Harley).knownPickups.empty() && restored.JoeKnownRoutes().empty(), "Pointing transfers no hidden pickup or route knowledge");
    Tick(restored, 31); Check(!Has(restored.CompanionOffers("harley", "F4_START", party), companion::Goal::InspectSignaledArea), "Old requests expire on shared clock"); Bytes(restored);
  }
  CampaignCore distrust(rules::Difficulty::Hard, 72); CompanionCommand abandon{Id(), CompanionOp::Abandon, "joe", "jack", "F4_START", 0}; abandon.physicallyVerified = true;
  Check(distrust.ApplyCompanion(abandon, Party("F4_START")).status == Status::Applied, "Observed abandonment reduces trust");
  auto call = Cmd(distrust, InteractionOp::Signal, "jack", "F4_START"); call.destination = call.node;
  Accepted(distrust, call, Context(call.node), false); Check(distrust.Interactions().orders.empty(), "Refusal commits a response without obeying");
  CampaignCore hidden(rules::Difficulty::Normal, 73); Joe(hidden, ActionKind::EnterHide, "H_F4_SHEETS", "F4_STORAGE");
  auto impossible = Cmd(hidden, InteractionOp::Gesture, "bartholomeu", "F4_START"); impossible.actor = "harley";
  Reject(hidden, impossible, Context("F4_START"));
  CampaignCore walking(rules::Difficulty::Normal, 74); auto follow = Cmd(walking, InteractionOp::Signal, "bartholomeu", "F4_START"); follow.destination = follow.node;
  Accepted(walking, follow, Context(follow.node)); CompanionCommand begin{Id(), CompanionOp::BeginTask, "bartholomeu", follow.eventId, follow.node, walking.Now()}; begin.argument = static_cast<int>(companion::Goal::FollowSignal); begin.physicallyVerified = true;
  Check(walking.ApplyCompanion(begin, Party(follow.node)).status == Status::Applied, "Walking companion has a current task");
  auto threat = Cmd(walking, InteractionOp::PerceiveThreat, "hunter", "F4_HALL_E"); threat.actor = "bartholomeu"; auto moving = Context(threat.node); moving.hunterPerceived = true;
  Accepted(walking, threat, moving); Load(walking, Bytes(walking)); Check(walking.NightRecord().size() == 1, "Companion's private perception during travel never becomes Joe's knowledge");
}
void QuietAndVariations() {
  std::set<unsigned> variants;
  for (unsigned seed = 1; seed <= 24; ++seed) {
    CampaignCore c(rules::Difficulty::Normal, seed); variants.insert(c.Interactions().variation);
    const auto planned = c.VariationFor("V_TRIAGE"); Check(planned && !planned->line.empty(), "Authored optional variant available");
    CampaignCore loaded(rules::Difficulty::Normal, seed + 1000); Load(loaded, Bytes(c)); Check(loaded.VariationFor("V_TRIAGE")->line == planned->line, "Load never rerolls optional dialogue");
  }
  Check(variants.size() == 3, "Campaign seeds exercise all three authored variants");
  CampaignCore c(rules::Difficulty::Normal, 80); auto ctx = Context("F1_TRIAGE");
  auto quiet = Cmd(c, InteractionOp::Signal, "harley", "F1_TRIAGE", 1); Accepted(c, quiet, ctx);
  auto banter = Cmd(c, InteractionOp::Banter, "V_TRIAGE", "F1_TRIAGE"); Reject(c, banter, ctx);
  c.SetPaused(true); Check(c.Tick(20, survival::Activity::Rest).status == Status::Paused && c.Now() == 0, "Pause consumes no quiet time");
  c.SetPaused(false); Load(c, Bytes(c)); Tick(c, 15); banter.atSeconds = c.Now();
  auto danger = ctx; danger.inDanger = true; Reject(c, banter, danger); Accepted(c, banter, ctx);
  auto duplicate = banter; duplicate.eventId = Id(); Reject(c, duplicate, ctx);
  auto ambient = Cmd(c, InteractionOp::Ambient, "V_TRIAGE", "F1_TRIAGE"); ctx.actualSourceMeters = {7,8,9};
  const auto world = c.World().objects; const auto result = c.ApplyInteraction(ambient, ctx);
  Check(result.status == Status::Applied && result.signal && result.signal->positionMeters.x == 7, "Physical optional occurrence returns its real sound source");
  Check(!c.Hunter().lastEstimate && c.World().objects.size() == world.size() && c.JoeKnownRoutes().empty(), "Optional occurrence changes no required clue or Hunter knowledge");
  Load(c, Bytes(c)); Check(c.NightRecord().size() == 3, "Record contains only confirmed signal, speech and occurrence");
}
void GesturesAndPressure() {
  CampaignCore c(rules::Difficulty::Normal, 90); const auto node = "F4_START"; auto ctx = Context(node);
  const auto fear = c.Companion(companion::Character::Bartholomeu).traits.fear;
  auto calm = Cmd(c, InteractionOp::Gesture, "bartholomeu", node); calm.actor = "harley"; Accepted(c, calm, ctx);
  const auto previewState = Bytes(c); const auto cue = c.InteractionCue(calm);
  Check(cue && cue->kind == director::CueKind::Gesture && cue->speaker == "harley" && Bytes(c) == previewState, "Gesture direction can be previewed without applying effects");
  Check(c.Companion(companion::Character::Bartholomeu).traits.fear < fear, "Harley physically calms Barth");
  auto spam = calm; spam.eventId = Id(); Reject(c, spam, ctx);
  auto absent = ctx; absent.actors.erase("joe"); Tick(c, 30); spam.atSeconds = c.Now(); Reject(c, spam, absent);
  Joe(c, ActionKind::Collect, "P_TREATMENT", "F1_PHARMACY"); Share(c, "bartholomeu", "treatment", node); Joe(c, ActionKind::Injury, "joe", node, 2);
  auto help = Cmd(c, InteractionOp::Gesture, "joe", node, 1); help.actor = "bartholomeu"; help.item = "treatment"; Accepted(c, help, ctx);
  Check(!c.Survival().bleeding && c.CompanionRuntime(companion::Character::Bartholomeu).inventory.contents.count("treatment") == 0, "Barth's help spends his own real treatment");
  Share(c, "jack", "magazine", node); const auto magazines = c.Inventory().contents.at("magazine");
  auto jack = Cmd(c, InteractionOp::Gesture, "joe", node, 2); jack.actor = "jack"; jack.item = "magazine"; Accepted(c, jack, ctx);
  Check(c.Inventory().contents.at("magazine") == magazines + 1 && c.Companion(companion::Character::Jack).traits.ammunition == 0, "Jack's awkward help transfers, never fabricates a magazine");
  Check(c.Narrative().witnessedClues.empty(), "Personal gestures expose no secret character clue"); Load(c, Bytes(c));
  CampaignCore pressure(rules::Difficulty::Hard, 91); auto threat = Cmd(pressure, InteractionOp::PerceiveThreat, "hunter", node); threat.actor = "bartholomeu";
  Reject(pressure, threat, ctx); ctx.hunterPerceived = true; Accepted(pressure, threat, ctx);
  const auto anxious = pressure.Companion(companion::Character::Bartholomeu).traits;
  Tick(pressure, 29); Check(pressure.Companion(companion::Character::Bartholomeu).traits.fear == anxious.fear, "Recent physical threat holds emotional pressure");
  CampaignCore resumed(rules::Difficulty::Hard, 992); Load(resumed, Bytes(pressure)); Tick(pressure, 11); Tick(resumed, 11);
  Check(Bytes(pressure) == Bytes(resumed), "Emotional timeline resumes deterministically");
  Check(pressure.Companion(companion::Character::Bartholomeu).traits.fear < anxious.fear && pressure.Companion(companion::Character::Bartholomeu).traits.trust == anxious.trust, "Rest settles temporary fear without erasing durable trust");
}
void Wedges() {
  CampaignCore c(rules::Difficulty::Normal, 100); Joe(c, ActionKind::Collect, "P_WINDOW_TOOL", "F2_SUPPLY");
  auto ctx = Context("F2_SERVICE"); auto place = Cmd(c, InteractionOp::PlaceWedge, "D_F2_SUPPLY", "F2_SERVICE"); place.item = "window_tool";
  auto trapped = ctx; trapped.alternativePathVerified = false; Reject(c, place, trapped); Accepted(c, place, ctx);
  Check(!c.Inventory().contents.count("window_tool") && c.World().objects.at(place.target).stateId == "wedged", "Wedge leaves an actual useful tool in the door");
  const auto before = Bytes(c); Check(c.Apply({Id(), ActionKind::OpenDoor, place.target, place.node, c.Now(), 0, true}).status == Status::MissingRequirement && Bytes(c) == before, "Ordinary opening cannot bypass wedge");
  CampaignCore loaded(rules::Difficulty::Normal, 9000); Load(loaded, before);
  Joe(loaded, ActionKind::Collect, "P_TREATMENT", "F1_PHARMACY"); Joe(loaded, ActionKind::Collect, "P_MAGAZINE", "F2_SUPPLY");
  Joe(loaded, ActionKind::Collect, "P_RADIO", "F1_RADIO"); Joe(loaded, ActionKind::Collect, "P_BATTERY", "F2_SERVER");
  auto recover = Cmd(loaded, InteractionOp::RecoverWedge, place.target, place.node); recover.item = place.item;
  Reject(loaded, recover, ctx); Check(loaded.Interactions().wedges.count(place.target), "Full bag leaves the actual wedge installed");
  Share(loaded, "harley", "magazine", place.node); Share(loaded, "bartholomeu", "magazine", place.node); Accepted(loaded, recover, ctx);
  Check(loaded.Inventory().contents.at("window_tool") == 1 && loaded.Interactions().wedges.empty(), "Recovery returns one tool and frees door");
  auto twice = recover; twice.eventId = Id(); Reject(loaded, twice, ctx);
  HunterContext hunter; hunter.actualHunterNode = place.node; hunter.reachableNodes.insert(place.node); hunter.physicallyVerified = hunter.lineOfSightVerified = hunter.reactionCueVerified = true;
  const auto knowsWedge=[&]{for(const auto& offer:c.HunterOffers(hunter))if(offer.goal==HunterGoal::BreakWedge)return true;return false;};
  Check(!knowsWedge(), "Unperceived installation does not grant Hunter a wedge target");
  Check(c.InspectEvidence(place.target, "wedged", {}, true), "Hunter actually notices wedged door");
  Check(c.HunterRuntime().aggression==0&&knowsWedge(), "Perceived wedge can be investigated at baseline pressure");
  for (int kind = 0; kind < 3; ++kind) { HunterCommand a; a.eventId = Id(); a.kind = HunterOp::Pressure; a.node = place.node; a.argument = kind; Check(c.ApplyHunter(a,hunter).status == Status::Applied, "Observed pressure permits environmental tactics"); }
  std::optional<HunterTask> task;
  for (int attempt = 0; attempt < 40 && !task; ++attempt) {
    HunterCommand choose; choose.eventId = Id(); choose.node = place.node;
    const auto result = c.ApplyHunter(choose,hunter); Check(result.status == Status::Applied && result.task, "Hunter chooses reachable tactic");
    if (result.task->goal == HunterGoal::BreakWedge) task = result.task;
    else { HunterCommand cancel; cancel.eventId = Id(); cancel.kind = HunterOp::Cancel; cancel.node = place.node; cancel.target = result.task->target; cancel.taskSerial = result.task->serial; Check(c.ApplyHunter(cancel,hunter).status == Status::Applied, "Unselected fixture tactic cancels"); }
  }
  Check(bool(task), "Hunter can plan a perceived wedge break");
  HunterCommand arrive; arrive.eventId = Id(); arrive.kind = HunterOp::Arrive; arrive.node = place.node; arrive.target = place.target; arrive.taskSerial = task->serial;
  Check(c.ApplyHunter(arrive,hunter).status == Status::Applied, "Break starts after physical arrival and warning");
  auto complete = arrive; complete.eventId = Id(); complete.kind = HunterOp::Complete;
  Tick(c, 5); complete.atSeconds = c.Now(); const auto waiting = Bytes(c);
  Check(c.ApplyHunter(complete,hunter).status == Status::MissingRequirement && Bytes(c) == waiting, "Wedge buys actual simulation time");
  CampaignCore race(rules::Difficulty::Normal, 101); Load(race, waiting);
  auto reclaimed = Cmd(race, InteractionOp::RecoverWedge, place.target, place.node); reclaimed.item = place.item; Accepted(race, reclaimed, ctx); Tick(race, 5);
  auto stale = complete; stale.atSeconds = race.Now(); const auto freed = Bytes(race);
  Check(race.ApplyHunter(stale,hunter).status == Status::MissingRequirement && Bytes(race) == freed, "Recovered wedge invalidates pending physical break without confiscating the tool");
  CampaignCore resumed(rules::Difficulty::Normal, 7000); Load(resumed, waiting); Tick(resumed, 5); complete.atSeconds = resumed.Now();
  const auto broken = resumed.ApplyHunter(complete,hunter);
  Check(broken.status == Status::Applied && broken.signal && resumed.Interactions().wedges.empty() && resumed.World().objects.at(place.target).stateId == "forced_open", "Verified break destroys wedge tool and opens passage");
  Check(!resumed.Inventory().contents.count("window_tool") && resumed.Ending() == rules::Ending::None, "Breaking grants no tool, damage, capture or ending"); Load(resumed, Bytes(resumed));
}
void WitnessedRecordAndRejection() {
  CampaignCore c(rules::Difficulty::Normal, 110); const auto node = "F2_SUPPLY";
  CompanionCommand outcome{Id(), CompanionOp::Outcome, "jack", "outcome", node, c.Now()}; outcome.argument = static_cast<int>(CompanionOutcome::Captured); outcome.physicallyVerified = true;
  auto isolated = Party(node); isolated.actors.erase("joe"); Check(c.ApplyCompanion(outcome, isolated).status == Status::Applied, "Offscreen physical capture persists world truth");
  Check(c.KnownCompanionFates().at("jack") == "paradeiro desconhecido" && c.NightRecord().empty(), "Joe's personal record does not read private companion outcome"); Load(c, Bytes(c));
  auto fate = Cmd(c, InteractionOp::WitnessFate, "jack", node); auto ctx = Context(node); ctx.actors.at("jack").active = false;
  auto remote = ctx; remote.actors.at("jack").node = "F4_START"; Reject(c, fate, remote); Accepted(c, fate, ctx);
  Check(c.KnownCompanionFates().at("jack") == "captura testemunhada" && c.NightRecord().size() == 1, "Real witness adds fate once");
  auto invented = Cmd(c, InteractionOp::WitnessFate, "harley", node, 1); Reject(c, invented, ctx);
  const auto good = Bytes(c); auto invalid = good; const auto mark = invalid.find("INTERACTIONS "); invalid[mark + std::string("INTERACTIONS ").size()] = '9';
  const auto checksum = invalid.rfind("CHECKSUM "); invalid = invalid.substr(0,checksum) + "CHECKSUM " + detail::Hash(invalid.substr(0,checksum)) + '\n';
  std::istringstream corrupt(invalid); Check(c.Load(corrupt) != save::Result::Ok && Bytes(c) == good, "Even with recomputed checksum, invalid variation is rejected atomically");
  auto old = good; old.replace(0,std::string("THE_THUG_RUNTIME_CAMPAIGN 8").size(),"THE_THUG_RUNTIME_CAMPAIGN 6"); std::istringstream legacy(old);
  Check(c.Load(legacy) == save::Result::UnsupportedVersion && Bytes(c) == good, "Runtime v6 cannot silently discard new interaction state");
  CampaignCore escape(rules::Difficulty::Normal, 111); Joe(escape, ActionKind::Trigger, "find_tunnel", "B1_TUNNEL"); Joe(escape, ActionKind::Trigger, "tunnel_exit", "X_TUNNEL");
  CompanionCommand together{Id(), CompanionOp::Outcome, "harley", "outcome", "X_TUNNEL", escape.Now()}; together.argument = static_cast<int>(CompanionOutcome::Together); together.physicallyVerified = true;
  Check(escape.ApplyCompanion(together, Party("X_TUNNEL")).status == Status::Applied && escape.KnownCompanionFates().at("harley") == "saiu comigo", "Physically crossing with Joe is inherently witnessed, including after campaign ending");
  Load(escape, Bytes(escape)); Check(escape.NightRecord().size() == 1, "Crossing together enters Joe's own record");
}
void QuietSuspendsOptionalPresentation() {
  CampaignCore c(rules::Difficulty::Normal, 120); director::Observation o; o.joeNode = "F4_START";
  for (const auto& name : {"harley", "bartholomeu", "jack", "thomas"}) o.companions.emplace(name, director::Actor{o.joeNode,true});
  Check(c.ApplyNarrative({Id(), ActionKind::BeginScene, "N01", o.joeNode, 0, 0, true}, o).status == Status::Applied, "Opening scene begins");
  while (const auto cue = c.CurrentPresentation(o)) {
    Action ack{Id(), ActionKind::SceneCue, cue->owner, o.joeNode, 0, cue->index, true}; ack.presentationSerial = cue->serial; ack.cueId = cue->cue.id;
    Check(c.ApplyNarrative(ack,o).status == Status::Applied, "Opening cue confirms");
  }
  Check(c.ApplyNarrative({Id(), ActionKind::BeginBanter, "BAN01", o.joeNode, 0, 0, true}, o).status == Status::Applied, "Optional conversation begins");
  const auto cue = c.CurrentPresentation(o); Check(bool(cue), "Optional line offered before quiet request");
  auto quiet = Cmd(c, InteractionOp::Signal, "bartholomeu", o.joeNode, 1); Accepted(c, quiet, Context(o.joeNode));
  Check(!c.CurrentPresentation(o) && !c.Narrative().activeBanter.empty() && c.Narrative().activeBanterLine == 0, "Quiet suspends existing optional line without marking it delivered");
  Action stale{Id(), ActionKind::BanterCue, cue->owner, o.joeNode, 0, cue->index, true}; stale.presentationSerial = cue->serial; stale.cueId = cue->cue.id;
  const auto before = Bytes(c); Check(c.ApplyNarrative(stale,o).status == Status::MissingRequirement && Bytes(c) == before, "Quiet request invalidates old speech callback");
  CampaignCore loaded(rules::Difficulty::Normal, 9120); Load(loaded, before); Check(!loaded.CurrentPresentation(o), "Quiet suspension persists through save");
  Tick(loaded, 15); o.nowSeconds = loaded.Now(); Check(loaded.CurrentPresentation(o)->cue.id == cue->cue.id, "After quiet expiry, resume first unconfirmed line");
  auto main = o; main.joeNode = "F4_OBSERVATION"; for (auto& [name, actor] : main.companions) { (void)name; actor.node = main.joeNode; }
  auto hush = Cmd(loaded, InteractionOp::Signal, "bartholomeu", main.joeNode, 1); Accepted(loaded, hush, Context(main.joeNode));
  Check(loaded.ApplyNarrative({Id(), ActionKind::BeginScene, "N02", main.joeNode, loaded.Now(), 0, true},main).status == Status::Applied && loaded.CurrentPresentation(main), "Discreet quiet request keeps main scene priority");
}
}
int main() {
  try { SignalsAndMovement(); QuietAndVariations(); GesturesAndPressure(); Wedges(); WitnessedRecordAndRejection(); QuietSuspendsOptionalPresentation(); std::cout << "Campaign interactions: " << checks << " checks passed\n"; }
  catch (const std::exception& e) { std::cerr << "Interaction failure after " << checks << ": " << e.what() << '\n'; return 1; }
}
