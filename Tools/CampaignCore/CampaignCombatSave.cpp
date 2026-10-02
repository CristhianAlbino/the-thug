#include "CampaignState.h"
#include "CombatIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
bool Tag(std::istream& in, const char* expected) {
  std::string tag; return bool(in >> tag) && tag == expected;
}
bool Time(double t, double now) { return std::isfinite(t) && t >= 0 && t <= now; }
bool Same(hunter::Vec3 a, hunter::Vec3 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
bool Creates(CombatOp op) { return op == CombatOp::BeginReload || op == CombatOp::BeginThrow || op == CombatOp::Fire; }
}
void CampaignCore::WriteCombat(std::ostream& out, const State& s) const {
  const auto& r = s.combat;
  out << std::setprecision(std::numeric_limits<double>::max_digits10);
  out << "COMBAT " << r.loadedRounds << ' ' << r.serial << ' ' << r.commandSerial << ' '
      << r.lastShotAt << ' ' << r.hunterBlockedUntil << ' ' << r.hunterImmuneUntil << ' '
      << r.oxygen << ' ' << r.gaspUntil << ' ' << r.holdingBreath << '\n';
  out << "COMBAT_TASK " << bool(r.task) << '\n';
  if (r.task) {
    const auto& t = *r.task;
    out << t.serial << ' ' << static_cast<int>(t.kind) << ' ' << std::quoted(t.item) << ' '
        << std::quoted(t.startEventId) << ' ' << std::quoted(t.node) << ' ' << t.startedAt << ' ' << t.readyAt << '\n';
  }
  out << "SHOTS " << r.shots.size() << '\n';
  for (const auto& [id, shot] : r.shots) {
    out << id << ' ' << std::quoted(shot.eventId) << ' ' << std::quoted(shot.node) << ' ' << shot.firedAt << ' ' << shot.expiresAt << ' ';
    detail::WriteVector(out, shot.origin); out << ' '; detail::WriteVector(out, shot.direction); out << '\n';
  }
  out << "THROWN " << r.thrown.size() << '\n';
  for (const auto& [id, item] : r.thrown) {
    out << id << ' ' << std::quoted(item.eventId) << ' ' << std::quoted(item.item) << ' ' << std::quoted(item.node)
        << ' ' << item.releasedAt << ' ' << item.detonateAt << ' ';
    detail::WriteVector(out, item.position); out << ' '; detail::WriteVector(out, item.velocity); out << '\n';
  }
  out << "SMOKE " << r.smoke.size() << '\n';
  for (const auto& [id, cloud] : r.smoke) {
    out << id << ' ' << std::quoted(cloud.eventId) << ' ' << std::quoted(cloud.node)
        << ' ' << cloud.startedAt << ' ' << cloud.expiresAt << ' ';
    detail::WriteVector(out, cloud.center); out << '\n';
  }
}
bool CampaignCore::ReadCombat(std::istream& in, State& s) const {
  CombatRuntimeSnapshot r;
  int holding, task;
  if (!Tag(in, "COMBAT") || !(in >> r.loadedRounds >> r.serial >> r.commandSerial >> r.lastShotAt >>
      r.hunterBlockedUntil >> r.hunterImmuneUntil >> r.oxygen >> r.gaspUntil >> holding) ||
      holding < 0 || holding > 1 || !Tag(in, "COMBAT_TASK") || !(in >> task) || task < 0 || task > 1) return false;
  r.holdingBreath = holding != 0;
  if (task) {
    CombatTask t; int kind;
    if (!(in >> t.serial >> kind >> std::quoted(t.item) >> std::quoted(t.startEventId) >> std::quoted(t.node)
        >> t.startedAt >> t.readyAt) || (kind != 0 && kind != 5)) return false;
    t.kind = static_cast<CombatOp>(kind); r.task = std::move(t);
  }
  std::size_t count;
  if (!Tag(in, "SHOTS") || !(in >> count) || count > 16) return false;
  for (std::size_t i = 0; i < count; ++i) {
    Shot shot;
    if (!(in >> shot.serial >> std::quoted(shot.eventId) >> std::quoted(shot.node) >> shot.firedAt >> shot.expiresAt) ||
        !detail::ReadVector(in, shot.origin) || !detail::ReadVector(in, shot.direction) || !r.shots.emplace(shot.serial, shot).second) return false;
  }
  if (!Tag(in, "THROWN") || !(in >> count) || count > 16) return false;
  for (std::size_t i = 0; i < count; ++i) {
    ThrownItem item;
    if (!(in >> item.serial >> std::quoted(item.eventId) >> std::quoted(item.item) >> std::quoted(item.node) >> item.releasedAt >> item.detonateAt) ||
        !detail::ReadVector(in, item.position) || !detail::ReadVector(in, item.velocity) || !r.thrown.emplace(item.serial, item).second) return false;
  }
  if (!Tag(in, "SMOKE") || !(in >> count) || count > 16) return false;
  for (std::size_t i = 0; i < count; ++i) {
    SmokeCloud cloud;
    if (!(in >> cloud.serial >> std::quoted(cloud.eventId) >> std::quoted(cloud.node) >> cloud.startedAt >> cloud.expiresAt) ||
        !detail::ReadVector(in, cloud.center) || !r.smoke.emplace(cloud.serial, cloud).second) return false;
  }
  s.combat = std::move(r);
  return true;
}

bool CampaignCore::CombatCoherent(const State& s) const {
  const auto& r = s.combat;
  const auto p = CombatProfile(s.routes.DifficultyLevel());
  const double now = s.world.NowSeconds();
  const bool finished = s.routes.Result() != rules::Ending::None;
  const auto effects = s.survival.CurrentEffects();
  if (r.loadedRounds < 0 || r.loadedRounds > p.magazineRounds || (r.loadedRounds && s.inventory.Quantity("rifle") != 1) ||
      !std::isfinite(r.oxygen) || r.oxygen < 0 || r.oxygen > p.oxygenCapacity ||
      !std::isfinite(r.gaspUntil) || r.gaspUntil < 0 || r.gaspUntil > now + p.gaspSeconds + 1e-8 ||
      (r.holdingBreath && (r.oxygen <= 0 || r.gaspUntil > now || effects.collapsed || effects.dead || finished)) ||
      r.shots.size() > 16 || r.thrown.size() + r.smoke.size() > 16 ||
      s.receipts.size() + detail::CombatReservedReceipts(r) > detail::MaxReceipts ||
      (finished && (r.task || !r.shots.empty() || !r.thrown.empty() || !r.smoke.empty())) ||
      ((effects.collapsed || effects.dead || !s.hidingPlace.empty()) && r.task)) return false;
  std::map<std::uint64_t, CombatCommand> commands, created;
  std::map<std::uint64_t, CombatCommand> completed, resolutions, detonations;
  std::set<std::uint64_t> terminal;
  std::set<std::string> destroyed;
  for (const auto& [id, signature] : s.receipts) if (signature.rfind("W ", 0) == 0) {
    CombatCommand c; c.eventId = id;
    std::uint64_t order, serial;
    if (!detail::ParseCombat(signature, c, order, serial) || !CombatStaticValid(c) || !Time(c.atSeconds, now) ||
        !order || !commands.emplace(order, c).second || (Creates(c.kind) ? !serial : serial != 0)) return false;
    if (serial && !created.emplace(serial, c).second) return false;
  }
  if (commands.size() != r.commandSerial || created.size() != r.serial) return false;
  std::uint64_t index = 0;
  for (const auto& [serial, c] : created) { (void)c; if (serial != ++index) return false; }
  index = 0;
  std::uint64_t creationsSeen = 0;
  int rounds = 0;
  double lastTime = 0, lastShot = -1, blocked = 0, immune = 0;
  for (const auto& [order, c] : commands) {
    if (order != ++index || c.atSeconds < lastTime) return false;
    lastTime = c.atSeconds;
    if (Creates(c.kind)) {
      const auto& expected = created.at(++creationsSeen);
      if (expected.eventId != c.eventId) return false;
    }
    if (c.kind == CombatOp::Fire) {
      if (rounds <= 0 || (lastShot >= 0 && c.atSeconds-lastShot < p.shotInterval)) return false;
      --rounds; lastShot = c.atSeconds;
    }
    if (c.serial) {
      const auto start = created.find(c.serial);
      if (start == created.end() || c.serial > creationsSeen || start->second.atSeconds > c.atSeconds) return false;
      const auto& began = start->second;
      if (c.kind == CombatOp::ResolveShot) {
        if (began.kind != CombatOp::Fire || c.atSeconds >= began.atSeconds + p.shotLifetime ||
            !resolutions.emplace(c.serial, c).second) return false;
        if (c.target != "miss" && c.target != "hunter") {
          const auto d = std::find_if(definition_->destructibles.begin(), definition_->destructibles.end(), [&](const auto& v) { return v.id == c.target; });
          if (d == definition_->destructibles.end() || !d->gunshot || d->node != c.node) return false;
          destroyed.insert(c.target);
        }
      } else if (c.kind == CombatOp::Detonate) {
        const auto released = completed.find(c.serial);
        if (released == completed.end() || released->second.kind != CombatOp::ReleaseThrow ||
            released->second.target != c.target || c.atSeconds != released->second.atSeconds +
            (c.target == "grenade" ? p.grenadeFuse : p.smokeFuse) || !detonations.emplace(c.serial, c).second) return false;
        for (const auto& hit : c.contacts) if (detail::CombatDistance(hit.position, c.position) > p.blastRadius) return false;
        for (const auto& hit : c.contacts) if (hit.target != "joe" && hit.target != "hunter" &&
            hit.target != "harley" && hit.target != "bartholomeu" && hit.target != "jack") {
          const auto d = std::find_if(definition_->destructibles.begin(), definition_->destructibles.end(), [&](const auto& v) { return v.id == hit.target; });
          if (d == definition_->destructibles.end() || !d->explosion || d->node != hit.node) return false;
          destroyed.insert(hit.target);
        }
      } else {
        const bool reload = c.kind == CombatOp::CompleteReload || c.kind == CombatOp::CancelReload;
        if (began.kind != (reload ? CombatOp::BeginReload : CombatOp::BeginThrow) || began.target != c.target ||
            !terminal.insert(c.serial).second) return false;
        if (c.kind == CombatOp::CompleteReload || c.kind == CombatOp::ReleaseThrow) {
          if (c.atSeconds < began.atSeconds + (reload ? p.reloadSeconds : p.throwSeconds)) return false;
          completed.emplace(c.serial, c);
          if (reload) rounds = p.magazineRounds;
        }
      }
    }
    bool hunterHit = c.kind == CombatOp::ResolveShot && c.target == "hunter";
    if (c.kind == CombatOp::Detonate) for (const auto& hit : c.contacts) hunterHit = hunterHit || hit.target == "hunter";
    if (hunterHit && c.atSeconds >= immune) {
      blocked = c.atSeconds + (c.kind == CombatOp::ResolveShot ? p.shotInterrupt : p.blastInterrupt);
      immune = blocked + p.interruptImmunity;
    }
  }
  if (rounds != r.loadedRounds || lastShot != r.lastShotAt || blocked != r.hunterBlockedUntil || immune != r.hunterImmuneUntil) return false;
  if (r.task) {
    const auto& t = *r.task;
    const auto found = created.find(t.serial);
    if (found == created.end() || t.serial != r.serial || terminal.count(t.serial) || !Time(t.startedAt, now)) return false;
    const auto& c = found->second;
    const bool reload = c.kind == CombatOp::BeginReload;
    if ((c.kind != CombatOp::BeginReload && c.kind != CombatOp::BeginThrow) || t.kind != c.kind ||
        t.item != (reload ? "magazine" : c.target) || t.node != c.node || t.startEventId != c.eventId ||
        t.startedAt != c.atSeconds || t.readyAt != c.atSeconds+(reload ? p.reloadSeconds : p.throwSeconds) ||
        s.inventory.Quantity(t.item) < 1 || (reload && s.inventory.Quantity("rifle") != 1)) return false;
  }
  for (const auto& [id, shot] : r.shots) {
    const auto found = created.find(id);
    if (found == created.end() || found->second.kind != CombatOp::Fire || resolutions.count(id)) return false;
    const auto& c = found->second;
    if (shot.serial != id || shot.eventId != c.eventId || shot.node != c.node || shot.firedAt != c.atSeconds ||
        shot.expiresAt != c.atSeconds+p.shotLifetime || shot.expiresAt <= now || !Same(shot.origin,c.position) || !Same(shot.direction,c.vector)) return false;
  }
  for (const auto& [id, c] : created) if (c.kind == CombatOp::Fire && !finished &&
      !resolutions.count(id) && now < c.atSeconds+p.shotLifetime && !r.shots.count(id)) return false;
  for (const auto& [id, item] : r.thrown) {
    const auto found = completed.find(id);
    if (found == completed.end() || found->second.kind != CombatOp::ReleaseThrow || detonations.count(id)) return false;
    const auto& c = found->second;
    if (item.serial != id || item.eventId != c.eventId || item.item != c.target || !definition_->nodes.count(item.node) ||
        !detail::CombatFinite(item.position) || !detail::CombatFinite(item.velocity) || item.releasedAt != c.atSeconds ||
        item.detonateAt != c.atSeconds+(c.target == "grenade" ? p.grenadeFuse : p.smokeFuse) || now > item.detonateAt) return false;
  }
  for (const auto& [id, c] : completed) if (c.kind == CombatOp::ReleaseThrow && !finished && !detonations.count(id) && !r.thrown.count(id)) return false;
  for (const auto& [id, cloud] : r.smoke) {
    const auto found = detonations.find(id);
    if (found == detonations.end() || found->second.target != "smoke") return false;
    const auto& c = found->second;
    if (cloud.serial != id || cloud.eventId != c.eventId || cloud.node != c.node || cloud.startedAt != c.atSeconds ||
        cloud.expiresAt != c.atSeconds+p.smokeSeconds || cloud.expiresAt <= now || !Same(cloud.center,c.position)) return false;
  }
  for (const auto& [id, c] : detonations) if (c.target == "smoke" && !finished && now < c.atSeconds+p.smokeSeconds && !r.smoke.count(id)) return false;
  for (const auto& d : definition_->destructibles) {
    const auto* object = s.world.Find(d.id);
    if (!object || !object->present || object->stateId != (destroyed.count(d.id) ? d.after : d.before)) return false;
  }
  return true;
}
} // namespace thug::campaign
