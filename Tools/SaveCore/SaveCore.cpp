#include "SaveCore.h"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace thug::save {
namespace {

constexpr std::size_t MaxFileBytes = 2 * 1024 * 1024;
constexpr unsigned MaxEntries = 10000;
constexpr std::size_t MaxIdBytes = 1024;
constexpr std::size_t MaxRandomBytes = 16000;
constexpr int Version = 1;

static_assert(std::is_nothrow_swappable_v<survival::Inventory>);
static_assert(std::is_nothrow_swappable_v<companion::CompanionCore>);
static_assert(std::is_nothrow_swappable_v<hunter::HunterCore>);
static_assert(std::is_nothrow_swappable_v<rules::RunRules>);
static_assert(std::is_nothrow_swappable_v<survival::SurvivalCore>);
static_assert(std::is_nothrow_swappable_v<perception::PerceptionCore>);
static_assert(std::is_nothrow_swappable_v<perception::EvidenceMemory>);
static_assert(std::is_nothrow_swappable_v<WorldState>);

bool CorrectRoster(const Party& party) {
  if (&party.harley == &party.bartholomeu ||
      &party.harley == &party.jack ||
      &party.bartholomeu == &party.jack ||
      party.harley.Identity() != companion::Character::Harley ||
      party.bartholomeu.Identity() != companion::Character::Bartholomeu ||
      party.jack.Identity() != companion::Character::Jack) return false;
  const auto a = party.harley.DifficultyLevel();
  return a == party.bartholomeu.DifficultyLevel() &&
         a == party.jack.DifficultyLevel();
}

bool CorrectCampaign(const Campaign& campaign) {
  return CorrectRoster(campaign.party) &&
         static_cast<int>(campaign.routes.DifficultyLevel()) ==
             static_cast<int>(campaign.party.harley.DifficultyLevel());
}

bool FitsId(const std::string& id) {
  return !id.empty() && id.size() <= MaxIdBytes;
}

bool FitsFormat(const survival::InventorySnapshot& inventory,
                const companion::CompanionSnapshot& harley,
                const companion::CompanionSnapshot& bartholomeu,
                const companion::CompanionSnapshot& jack) {
  if (inventory.definitions.size() > MaxEntries ||
      inventory.contents.size() > MaxEntries) return false;
  for (const auto& item : inventory.definitions)
    if (!FitsId(item.id)) return false;
  for (const auto& [id, count] : inventory.contents)
    if (!FitsId(id)) return false;
  for (const auto* companion : {&harley, &bartholomeu, &jack}) {
    if (companion->knownRoutes.size() > MaxEntries ||
        companion->appliedEvents.size() > MaxEntries ||
        companion->randomState.empty() ||
        companion->randomState.size() > MaxRandomBytes) return false;
    for (const auto& id : companion->knownRoutes)
      if (!FitsId(id)) return false;
    for (const auto& id : companion->appliedEvents)
      if (!FitsId(id)) return false;
  }
  return true;
}

std::string Checksum(const std::string& bytes) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (unsigned char byte : bytes) {
    hash ^= byte;
    hash *= 1099511628211ULL;
  }
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::hex << std::setfill('0') << std::setw(16) << hash;
  return out.str();
}

bool Tag(std::istream& in, const char* expected) {
  std::string value;
  return static_cast<bool>(in >> value) && value == expected;
}

bool ReadString(std::istream& in, std::string& value,
                std::size_t maxLength = MaxIdBytes, bool allowEmpty = false) {
  in >> std::ws;
  if (in.peek() != '"' || !(in >> std::quoted(value))) return false;
  return (allowEmpty || !value.empty()) && value.size() <= maxLength;
}

bool ReadCount(std::istream& in, unsigned& count) {
  return static_cast<bool>(in >> count) && count <= MaxEntries;
}

bool ReadBool(std::istream& in, bool& value) {
  int number;
  if (!(in >> number) || (number != 0 && number != 1)) return false;
  value = number == 1;
  return true;
}

bool ValidCategory(int value) { return value >= 0 && value <= 5; }
bool ValidCharacter(int value) { return value >= 0 && value <= 2; }
bool ValidDifficulty(int value) { return value >= 0 && value <= 3; }

void WriteInventory(std::ostream& out,
                    const survival::InventorySnapshot& snapshot) {
  out << "INVENTORY " << snapshot.capacitySlots << ' '
      << snapshot.definitions.size() << ' ' << snapshot.contents.size() << '\n';
  for (const auto& item : snapshot.definitions) {
    out << "ITEM " << std::quoted(item.id) << ' '
        << static_cast<int>(item.category) << ' ' << item.slotCost << ' '
        << item.maxQuantity << ' ' << static_cast<int>(item.droppable) << '\n';
  }
  for (const auto& [id, count] : snapshot.contents) {
    out << "STOCK " << std::quoted(id) << ' ' << count << '\n';
  }
}

bool ReadInventory(std::istream& in, survival::InventorySnapshot& snapshot) {
  unsigned definitions, contents;
  if (!Tag(in, "INVENTORY") || !(in >> snapshot.capacitySlots) ||
      !ReadCount(in, definitions) || !ReadCount(in, contents) ||
      snapshot.capacitySlots < 0) return false;
  for (unsigned i = 0; i < definitions; ++i) {
    survival::ItemDefinition item;
    int category;
    if (!Tag(in, "ITEM") || !ReadString(in, item.id) ||
        !(in >> category >> item.slotCost >> item.maxQuantity) ||
        !ReadBool(in, item.droppable) || !ValidCategory(category) ||
        item.slotCost <= 0 || item.maxQuantity <= 0) return false;
    item.category = static_cast<survival::ItemCategory>(category);
    snapshot.definitions.push_back(std::move(item));
  }
  for (unsigned i = 0; i < contents; ++i) {
    std::string id;
    int count;
    if (!Tag(in, "STOCK") || !ReadString(in, id) || !(in >> count) ||
        count <= 0 || !snapshot.contents.emplace(std::move(id), count).second)
      return false;
  }
  return true;
}

void WriteCompanion(std::ostream& out,
                    const companion::CompanionSnapshot& snapshot) {
  const auto& t = snapshot.traits;
  out << "COMPANION " << static_cast<int>(snapshot.character) << ' '
      << static_cast<int>(snapshot.difficulty) << '\n'
      << "TRAITS " << t.trust << ' ' << t.fear << ' ' << t.stress << ' '
      << t.loyalty << ' ' << t.selfPreservation << ' ' << t.relationship << ' '
      << t.ammunition << ' ' << static_cast<int>(t.injured) << ' '
      << snapshot.nowSeconds << '\n'
      << "RNG " << std::quoted(snapshot.randomState) << '\n'
      << "ROUTES " << snapshot.knownRoutes.size() << '\n';
  for (const auto& id : snapshot.knownRoutes)
    out << "ROUTE " << std::quoted(id) << '\n';
  out << "EVENTS " << snapshot.appliedEvents.size() << '\n';
  for (const auto& id : snapshot.appliedEvents)
    out << "EVENT " << std::quoted(id) << '\n';
}

bool ReadCompanion(std::istream& in,
                   companion::CompanionSnapshot& snapshot) {
  int character, difficulty;
  if (!Tag(in, "COMPANION") || !(in >> character >> difficulty) ||
      !ValidCharacter(character) || !ValidDifficulty(difficulty)) return false;
  snapshot.character = static_cast<companion::Character>(character);
  snapshot.difficulty = static_cast<companion::Difficulty>(difficulty);
  auto& t = snapshot.traits;
  if (!Tag(in, "TRAITS") ||
      !(in >> t.trust >> t.fear >> t.stress >> t.loyalty >>
        t.selfPreservation >> t.relationship >> t.ammunition) ||
      !ReadBool(in, t.injured) || !(in >> snapshot.nowSeconds) ||
      !Tag(in, "RNG") || !ReadString(in, snapshot.randomState, MaxRandomBytes))
    return false;
  unsigned routes;
  if (!Tag(in, "ROUTES") || !ReadCount(in, routes)) return false;
  for (unsigned i = 0; i < routes; ++i) {
    std::string id;
    if (!Tag(in, "ROUTE") || !ReadString(in, id) ||
        !snapshot.knownRoutes.insert(std::move(id)).second) return false;
  }
  unsigned events;
  if (!Tag(in, "EVENTS") || !ReadCount(in, events)) return false;
  for (unsigned i = 0; i < events; ++i) {
    std::string id;
    if (!Tag(in, "EVENT") || !ReadString(in, id) ||
        !snapshot.appliedEvents.insert(std::move(id)).second) return false;
  }
  return true;
}

Result ReadBounded(std::istream& input, std::string& bytes) {
  char c;
  while (input.get(c)) {
    if (bytes.size() == MaxFileBytes) return Result::TooLarge;
    bytes.push_back(c);
  }
  return input.eof() && !input.bad() ? Result::Ok : Result::IoError;
}

bool FitsCampaign(const hunter::HunterSnapshot& hunter,
                  const perception::PerceptionSnapshot& hearing,
                  const perception::EvidenceMemorySnapshot& evidence,
                  const WorldSnapshot& world) {
  if (hunter.lastHeardType.size() > MaxIdBytes ||
      hunter.randomState.empty() || hunter.randomState.size() > MaxRandomBytes ||
      hearing.randomState.empty() || hearing.randomState.size() > MaxRandomBytes ||
      hunter.observedRoutes.size() > MaxEntries ||
      hunter.confirmedHideSpots.size() > MaxEntries ||
      evidence.lastSeen.size() > MaxEntries ||
      !FitsId(world.levelId) || world.catalog.size() > MaxEntries ||
      world.objects.size() > MaxEntries) return false;
  for (const auto& [id, entry] : hunter.observedRoutes)
    if (!FitsId(id)) return false;
  for (const auto& [id, entry] : hunter.confirmedHideSpots)
    if (!FitsId(id)) return false;
  for (const auto& [id, state] : evidence.lastSeen)
    if (!FitsId(id) || !FitsId(state)) return false;
  for (const auto& object : world.catalog)
    if (!FitsId(object.id) || !FitsId(object.archetype) ||
        !FitsId(object.initial.stateId)) return false;
  for (const auto& [id, state] : world.objects)
    if (!FitsId(id) || !FitsId(state.stateId)) return false;
  return true;
}

bool ClockConsistent(const WorldSnapshot& world,
                     const hunter::HunterSnapshot& hunter,
                     const rules::RunSnapshot& routes,
                     const companion::CompanionSnapshot& harley,
                     const companion::CompanionSnapshot& bartholomeu,
                     const companion::CompanionSnapshot& jack,
                     const perception::EvidenceMemorySnapshot& evidence) {
  const double now = world.nowSeconds;
  return std::isfinite(now) && now >= 0 &&
         hunter.nowSeconds <= now && routes.nowSeconds <= now &&
         harley.nowSeconds <= now && bartholomeu.nowSeconds <= now &&
         jack.nowSeconds <= now && evidence.nowSeconds <= now;
}

bool OutcomeConsistent(const survival::SurvivalSnapshot& survival,
                       const rules::RunSnapshot& routes) {
  // Other causes can end Joe's run without fatal blood loss, but the reverse
  // cannot be saved before the coordinator has delivered JoeDied.
  return !survival.dead || routes.result == rules::Ending::Died;
}

void WriteHunter(std::ostream& out, const hunter::HunterSnapshot& s) {
  const auto& t = s.tuning;
  out << "HUNTER " << static_cast<int>(s.state) << ' ' << s.nowSeconds << ' '
      << s.enteredStateAt << ' ' << static_cast<int>(s.lastEstimate.has_value())
      << '\n'
      << "HUNTER_TUNING " << t.minimumSoundRadiusMeters << ' '
      << t.soundThreshold << ' ' << t.searchSeconds << ' '
      << t.investigateSeconds << ' ' << t.memoryHalfLifeSeconds << ' '
      << t.locationLifetimeSeconds << ' ' << t.minimumRouteWeight << ' '
      << t.routeEvidenceWeight << ' ' << t.hideSuspicionCap << '\n';
  if (s.lastEstimate) {
    const auto& e = *s.lastEstimate;
    out << "ESTIMATE " << e.center.x << ' ' << e.center.y << ' '
        << e.center.z << ' ' << e.radiusMeters << ' '
        << e.observedAtSeconds << '\n';
  }
  out << "HEARD " << std::quoted(s.lastHeardType) << '\n'
      << "ROUTE_EVIDENCE " << s.observedRoutes.size() << '\n';
  for (const auto& [id, e] : s.observedRoutes)
    out << "ROUTE_MEMORY " << std::quoted(id) << ' ' << e.score << ' '
        << e.lastObservedAt << ' ' << e.observations << '\n';
  out << "HIDE_EVIDENCE " << s.confirmedHideSpots.size() << '\n';
  for (const auto& [id, e] : s.confirmedHideSpots)
    out << "HIDE_MEMORY " << std::quoted(id) << ' ' << e.score << ' '
        << e.lastObservedAt << ' ' << e.observations << '\n';
  out << "HUNTER_RNG " << std::quoted(s.randomState) << '\n';
}

bool ReadHunter(std::istream& in, hunter::HunterSnapshot& s) {
  int state;
  bool hasEstimate;
  if (!Tag(in, "HUNTER") || !(in >> state >> s.nowSeconds >> s.enteredStateAt) ||
      !ReadBool(in, hasEstimate) || state < 0 || state > 3 ||
      !Tag(in, "HUNTER_TUNING")) return false;
  s.state = static_cast<hunter::State>(state);
  auto& t = s.tuning;
  if (!(in >> t.minimumSoundRadiusMeters >> t.soundThreshold >>
        t.searchSeconds >> t.investigateSeconds >> t.memoryHalfLifeSeconds >>
        t.locationLifetimeSeconds >> t.minimumRouteWeight >>
        t.routeEvidenceWeight >> t.hideSuspicionCap)) return false;
  if (hasEstimate) {
    hunter::LocationEstimate e;
    if (!Tag(in, "ESTIMATE") ||
        !(in >> e.center.x >> e.center.y >> e.center.z >>
          e.radiusMeters >> e.observedAtSeconds)) return false;
    s.lastEstimate = e;
  }
  if (!Tag(in, "HEARD") ||
      !ReadString(in, s.lastHeardType, MaxIdBytes, true)) return false;
  unsigned count;
  if (!Tag(in, "ROUTE_EVIDENCE") || !ReadCount(in, count)) return false;
  for (unsigned i = 0; i < count; ++i) {
    std::string id;
    hunter::EvidenceSnapshot e;
    if (!Tag(in, "ROUTE_MEMORY") || !ReadString(in, id) ||
        !(in >> e.score >> e.lastObservedAt >> e.observations) ||
        !s.observedRoutes.emplace(std::move(id), e).second) return false;
  }
  if (!Tag(in, "HIDE_EVIDENCE") || !ReadCount(in, count)) return false;
  for (unsigned i = 0; i < count; ++i) {
    std::string id;
    hunter::EvidenceSnapshot e;
    if (!Tag(in, "HIDE_MEMORY") || !ReadString(in, id) ||
        !(in >> e.score >> e.lastObservedAt >> e.observations) ||
        !s.confirmedHideSpots.emplace(std::move(id), e).second) return false;
  }
  return Tag(in, "HUNTER_RNG") &&
         ReadString(in, s.randomState, MaxRandomBytes);
}

void WriteRules(std::ostream& out, const rules::RunSnapshot& s) {
  out << "RUN " << static_cast<int>(s.difficulty) << ' '
      << static_cast<int>(s.result) << ' ' << s.nowSeconds << ' '
      << static_cast<int>(s.helicopterDeadline.has_value()) << '\n'
      << "GARAGE " << s.safeOpened << ' ' << s.garageKey << ' '
      << s.garageGate << ' ' << s.garageFailed << '\n'
      << "ROOF " << s.radioFound << ' ' << s.batteriesInstalled << ' '
      << s.radioContact << ' ' << s.roofFailed << '\n';
  if (s.helicopterDeadline)
    out << "DEADLINE " << *s.helicopterDeadline << '\n';
  out << "TUNNEL " << s.tunnelFound << ' ' << s.tunnelFailed << '\n'
      << "WINDOW " << s.windowFound << ' ' << s.windowTools << ' '
      << s.windowUnblocked << ' ' << s.windowFailed << '\n';
}

bool ReadRules(std::istream& in, rules::RunSnapshot& s) {
  int difficulty, ending;
  bool deadline;
  if (!Tag(in, "RUN") || !(in >> difficulty >> ending >> s.nowSeconds) ||
      !ReadBool(in, deadline) || !ValidDifficulty(difficulty) ||
      ending < 0 || ending > 8) return false;
  s.difficulty = static_cast<rules::Difficulty>(difficulty);
  s.result = static_cast<rules::Ending>(ending);
  if (!Tag(in, "GARAGE") || !ReadBool(in, s.safeOpened) ||
      !ReadBool(in, s.garageKey) || !ReadBool(in, s.garageGate) ||
      !ReadBool(in, s.garageFailed) || !Tag(in, "ROOF") ||
      !ReadBool(in, s.radioFound) || !ReadBool(in, s.batteriesInstalled) ||
      !ReadBool(in, s.radioContact) || !ReadBool(in, s.roofFailed)) return false;
  if (deadline) {
    double value;
    if (!Tag(in, "DEADLINE") || !(in >> value)) return false;
    s.helicopterDeadline = value;
  }
  return Tag(in, "TUNNEL") && ReadBool(in, s.tunnelFound) &&
         ReadBool(in, s.tunnelFailed) && Tag(in, "WINDOW") &&
         ReadBool(in, s.windowFound) && ReadBool(in, s.windowTools) &&
         ReadBool(in, s.windowUnblocked) && ReadBool(in, s.windowFailed);
}

void WriteSurvival(std::ostream& out, const survival::SurvivalSnapshot& s) {
  const auto& t = s.tuning;
  out << "SURVIVAL_TUNING " << t.maxStamina << ' '
      << t.sprintDrainPerSecond << ' ' << t.restRecoveryPerSecond << ' '
      << t.walkRecoveryFraction << ' ' << t.moderateDrainMultiplier << ' '
      << t.severeDrainMultiplier << ' ' << t.moderateRecoveryMultiplier << ' '
      << t.severeRecoveryMultiplier << ' ' << t.exhaustionReleaseFraction << ' '
      << t.bleedingPerSecond << ' ' << t.collapseBloodLoss << ' '
      << t.fatalBloodLoss << ' ' << t.treatmentBloodRecovery << '\n'
      << "SURVIVAL " << static_cast<int>(s.injury) << ' ' << s.stamina << ' '
      << s.bloodLoss << ' ' << s.bleeding << ' ' << s.exhausted << ' '
      << s.collapsed << ' ' << s.dead << '\n';
}

bool ReadSurvival(std::istream& in, survival::SurvivalSnapshot& s) {
  auto& t = s.tuning;
  if (!Tag(in, "SURVIVAL_TUNING") ||
      !(in >> t.maxStamina >> t.sprintDrainPerSecond >>
        t.restRecoveryPerSecond >> t.walkRecoveryFraction >>
        t.moderateDrainMultiplier >> t.severeDrainMultiplier >>
        t.moderateRecoveryMultiplier >> t.severeRecoveryMultiplier >>
        t.exhaustionReleaseFraction >> t.bleedingPerSecond >>
        t.collapseBloodLoss >> t.fatalBloodLoss >> t.treatmentBloodRecovery))
    return false;
  int injury;
  if (!Tag(in, "SURVIVAL") || !(in >> injury >> s.stamina >> s.bloodLoss) ||
      injury < 0 || injury > 3 || !ReadBool(in, s.bleeding) ||
      !ReadBool(in, s.exhausted) || !ReadBool(in, s.collapsed) ||
      !ReadBool(in, s.dead)) return false;
  s.injury = static_cast<survival::Injury>(injury);
  return true;
}

void WritePerception(std::ostream& out,
                     const perception::PerceptionSnapshot& s) {
  const auto& t = s.tuning;
  out << "PERCEPTION_TUNING " << t.minimumEstimateRadiusMeters << ' '
      << t.minimumAudibleStrength << ' ' << t.distanceUncertaintyFactor << ' '
      << t.occlusionUncertaintyMeters << ' ' << t.maskingUncertaintyMeters
      << '\n' << "PERCEPTION_RNG " << std::quoted(s.randomState) << '\n';
}

bool ReadPerception(std::istream& in,
                    perception::PerceptionSnapshot& s) {
  auto& t = s.tuning;
  return Tag(in, "PERCEPTION_TUNING") &&
         static_cast<bool>(in >> t.minimumEstimateRadiusMeters >>
                           t.minimumAudibleStrength >>
                           t.distanceUncertaintyFactor >>
                           t.occlusionUncertaintyMeters >>
                           t.maskingUncertaintyMeters) &&
         Tag(in, "PERCEPTION_RNG") &&
         ReadString(in, s.randomState, MaxRandomBytes);
}

void WriteEvidence(std::ostream& out,
                   const perception::EvidenceMemorySnapshot& s) {
  out << "INSPECTED " << s.nowSeconds << ' ' << s.lastSeen.size() << '\n';
  for (const auto& [id, state] : s.lastSeen)
    out << "SEEN " << std::quoted(id) << ' ' << std::quoted(state) << '\n';
}

bool ReadEvidence(std::istream& in,
                  perception::EvidenceMemorySnapshot& s) {
  unsigned count;
  if (!Tag(in, "INSPECTED") || !(in >> s.nowSeconds) ||
      !ReadCount(in, count)) return false;
  for (unsigned i = 0; i < count; ++i) {
    std::string id, state;
    if (!Tag(in, "SEEN") || !ReadString(in, id) ||
        !ReadString(in, state) ||
        !s.lastSeen.emplace(std::move(id), std::move(state)).second)
      return false;
  }
  return true;
}

void WriteWorldObject(std::ostream& out, const WorldObjectState& state) {
  out << std::quoted(state.stateId) << ' '
      << state.positionMeters.x << ' ' << state.positionMeters.y << ' '
      << state.positionMeters.z << ' ' << state.rotationDegrees.x << ' '
      << state.rotationDegrees.y << ' ' << state.rotationDegrees.z << ' '
      << state.present << '\n';
}

bool ReadWorldObject(std::istream& in, WorldObjectState& state) {
  return ReadString(in, state.stateId) &&
         static_cast<bool>(in >> state.positionMeters.x >>
                           state.positionMeters.y >> state.positionMeters.z >>
                           state.rotationDegrees.x >> state.rotationDegrees.y >>
                           state.rotationDegrees.z) &&
         ReadBool(in, state.present);
}

void WriteWorld(std::ostream& out, const WorldSnapshot& s) {
  out << "WORLD " << std::quoted(s.levelId) << ' ' << s.nowSeconds << ' '
      << s.catalog.size() << ' ' << s.objects.size() << '\n';
  for (const auto& definition : s.catalog) {
    out << "WORLD_DEF " << std::quoted(definition.id) << ' '
        << std::quoted(definition.archetype) << ' ';
    WriteWorldObject(out, definition.initial);
  }
  for (const auto& [id, state] : s.objects) {
    out << "WORLD_OBJECT " << std::quoted(id) << ' ';
    WriteWorldObject(out, state);
  }
}

bool ReadWorld(std::istream& in, WorldSnapshot& s) {
  unsigned definitions, objects;
  if (!Tag(in, "WORLD") || !ReadString(in, s.levelId) ||
      !(in >> s.nowSeconds) || !ReadCount(in, definitions) ||
      !ReadCount(in, objects)) return false;
  for (unsigned i = 0; i < definitions; ++i) {
    WorldObjectDefinition definition;
    if (!Tag(in, "WORLD_DEF") || !ReadString(in, definition.id) ||
        !ReadString(in, definition.archetype) ||
        !ReadWorldObject(in, definition.initial)) return false;
    s.catalog.push_back(std::move(definition));
  }
  for (unsigned i = 0; i < objects; ++i) {
    std::string id;
    WorldObjectState state;
    if (!Tag(in, "WORLD_OBJECT") || !ReadString(in, id) ||
        !ReadWorldObject(in, state) ||
        !s.objects.emplace(std::move(id), std::move(state)).second)
      return false;
  }
  return true;
}

} // namespace

Result SaveParty(std::ostream& output, const Party& party) {
  if (!CorrectRoster(party)) return Result::IncompatibleRoster;
  const auto inventory = party.joe.CaptureSnapshot();
  const auto harley = party.harley.CaptureSnapshot();
  const auto bartholomeu = party.bartholomeu.CaptureSnapshot();
  const auto jack = party.jack.CaptureSnapshot();
  if (!FitsFormat(inventory, harley, bartholomeu, jack))
    return Result::TooLarge;
  std::ostringstream buffer;
  buffer.imbue(std::locale::classic());
  buffer << std::setprecision(std::numeric_limits<double>::max_digits10)
         << "THE_THUG_PARTY " << Version << '\n';
  WriteInventory(buffer, inventory);
  WriteCompanion(buffer, harley);
  WriteCompanion(buffer, bartholomeu);
  WriteCompanion(buffer, jack);
  buffer << "END\n";
  const auto payload = buffer.str();
  const auto data = payload + "CHECKSUM " + Checksum(payload) + '\n';
  if (data.size() > MaxFileBytes) return Result::TooLarge;
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  return output ? Result::Ok : Result::IoError;
}

Result LoadParty(std::istream& input, Party party) {
  if (!CorrectRoster(party)) return Result::IncompatibleRoster;
  std::string data;
  const auto readResult = ReadBounded(input, data);
  if (readResult != Result::Ok) return readResult;
  std::istringstream in(data);
  in.imbue(std::locale::classic());
  int version;
  if (!Tag(in, "THE_THUG_PARTY") || !(in >> version)) return Result::Malformed;
  if (version != Version) return Result::UnsupportedVersion;

  survival::InventorySnapshot inventory;
  companion::CompanionSnapshot harley, bartholomeu, jack;
  if (!ReadInventory(in, inventory) || !ReadCompanion(in, harley) ||
      !ReadCompanion(in, bartholomeu) || !ReadCompanion(in, jack) ||
      !Tag(in, "END")) return Result::Malformed;
  std::string endLine;
  if (!std::getline(in, endLine) || !endLine.empty()) return Result::Malformed;
  const auto checksumOffset = in.tellg();
  if (checksumOffset < 0) return Result::Malformed;
  std::string checksum;
  if (!Tag(in, "CHECKSUM") || !(in >> checksum) || checksum.size() != 16)
    return Result::Malformed;
  in >> std::ws;
  if (!in.eof()) return Result::Malformed;
  if (checksum != Checksum(data.substr(0,
          static_cast<std::size_t>(checksumOffset)))) return Result::Corrupt;

  // Stage all changes; a failure in any state leaves every live object intact.
  auto stagedJoe = party.joe;
  auto stagedHarley = party.harley;
  auto stagedBartholomeu = party.bartholomeu;
  auto stagedJack = party.jack;
  if (!stagedJoe.RestoreSnapshot(inventory) ||
      !stagedHarley.RestoreSnapshot(harley) ||
      !stagedBartholomeu.RestoreSnapshot(bartholomeu) ||
      !stagedJack.RestoreSnapshot(jack)) return Result::IncompatibleRoster;

  using std::swap;
  swap(party.joe, stagedJoe);
  swap(party.harley, stagedHarley);
  swap(party.bartholomeu, stagedBartholomeu);
  swap(party.jack, stagedJack);
  return Result::Ok;
}

Result SaveCampaign(std::ostream& output, const Campaign& campaign) {
  if (!CorrectCampaign(campaign)) return Result::IncompatibleRoster;
  const auto inventory = campaign.party.joe.CaptureSnapshot();
  const auto harley = campaign.party.harley.CaptureSnapshot();
  const auto bartholomeu = campaign.party.bartholomeu.CaptureSnapshot();
  const auto jack = campaign.party.jack.CaptureSnapshot();
  const auto hunter = campaign.hunter.CaptureSnapshot();
  const auto routes = campaign.routes.CaptureSnapshot();
  const auto survival = campaign.survival.CaptureSnapshot();
  const auto hearing = campaign.hearing.CaptureSnapshot();
  const auto evidence = campaign.evidence.CaptureSnapshot();
  const auto world = campaign.world.CaptureSnapshot();
  if (!ClockConsistent(world, hunter, routes, harley, bartholomeu,
                       jack, evidence) || !OutcomeConsistent(survival, routes))
    return Result::IncompatibleRoster;
  if (!FitsFormat(inventory, harley, bartholomeu, jack) ||
      !FitsCampaign(hunter, hearing, evidence, world)) return Result::TooLarge;

  std::ostringstream buffer;
  buffer.imbue(std::locale::classic());
  buffer << std::setprecision(std::numeric_limits<double>::max_digits10)
         << "THE_THUG_CAMPAIGN 1\n";
  WriteInventory(buffer, inventory);
  WriteCompanion(buffer, harley);
  WriteCompanion(buffer, bartholomeu);
  WriteCompanion(buffer, jack);
  WriteHunter(buffer, hunter);
  WriteRules(buffer, routes);
  WriteSurvival(buffer, survival);
  WritePerception(buffer, hearing);
  WriteEvidence(buffer, evidence);
  WriteWorld(buffer, world);
  buffer << "END\n";
  const auto payload = buffer.str();
  const auto data = payload + "CHECKSUM " + Checksum(payload) + '\n';
  if (data.size() > MaxFileBytes) return Result::TooLarge;
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  return output ? Result::Ok : Result::IoError;
}

Result LoadCampaign(std::istream& input, Campaign campaign) {
  if (!CorrectCampaign(campaign)) return Result::IncompatibleRoster;
  std::string data;
  const auto readResult = ReadBounded(input, data);
  if (readResult != Result::Ok) return readResult;
  std::istringstream in(data);
  in.imbue(std::locale::classic());
  int version;
  if (!Tag(in, "THE_THUG_CAMPAIGN") || !(in >> version))
    return Result::Malformed;
  if (version != 1) return Result::UnsupportedVersion;

  survival::InventorySnapshot inventory;
  companion::CompanionSnapshot harley, bartholomeu, jack;
  hunter::HunterSnapshot hunter;
  rules::RunSnapshot routes;
  survival::SurvivalSnapshot survival;
  perception::PerceptionSnapshot hearing;
  perception::EvidenceMemorySnapshot evidence;
  WorldSnapshot world;
  if (!ReadInventory(in, inventory) || !ReadCompanion(in, harley) ||
      !ReadCompanion(in, bartholomeu) || !ReadCompanion(in, jack) ||
      !ReadHunter(in, hunter) || !ReadRules(in, routes) ||
      !ReadSurvival(in, survival) || !ReadPerception(in, hearing) ||
      !ReadEvidence(in, evidence) || !ReadWorld(in, world) ||
      !Tag(in, "END")) return Result::Malformed;
  std::string endLine;
  if (!std::getline(in, endLine) || !endLine.empty()) return Result::Malformed;
  const auto checksumOffset = in.tellg();
  if (checksumOffset < 0) return Result::Malformed;
  std::string checksum;
  if (!Tag(in, "CHECKSUM") || !(in >> checksum) || checksum.size() != 16)
    return Result::Malformed;
  in >> std::ws;
  if (!in.eof()) return Result::Malformed;
  if (checksum != Checksum(data.substr(0,
          static_cast<std::size_t>(checksumOffset)))) return Result::Corrupt;

  if (!ClockConsistent(world, hunter, routes, harley, bartholomeu,
                       jack, evidence) || !OutcomeConsistent(survival, routes))
    return Result::IncompatibleRoster;

  auto stagedJoe = campaign.party.joe;
  auto stagedHarley = campaign.party.harley;
  auto stagedBartholomeu = campaign.party.bartholomeu;
  auto stagedJack = campaign.party.jack;
  auto stagedHunter = campaign.hunter;
  auto stagedRoutes = campaign.routes;
  auto stagedSurvival = campaign.survival;
  auto stagedHearing = campaign.hearing;
  auto stagedEvidence = campaign.evidence;
  auto stagedWorld = campaign.world;
  if (!stagedJoe.RestoreSnapshot(inventory) ||
      !stagedHarley.RestoreSnapshot(harley) ||
      !stagedBartholomeu.RestoreSnapshot(bartholomeu) ||
      !stagedJack.RestoreSnapshot(jack) ||
      !stagedHunter.RestoreSnapshot(hunter) ||
      !stagedRoutes.RestoreSnapshot(routes) ||
      !stagedSurvival.RestoreSnapshot(survival) ||
      !stagedHearing.RestoreSnapshot(hearing) ||
      !stagedEvidence.RestoreSnapshot(evidence) ||
      !stagedWorld.RestoreSnapshot(world))
    return Result::IncompatibleRoster;

  using std::swap;
  swap(campaign.party.joe, stagedJoe);
  swap(campaign.party.harley, stagedHarley);
  swap(campaign.party.bartholomeu, stagedBartholomeu);
  swap(campaign.party.jack, stagedJack);
  swap(campaign.hunter, stagedHunter);
  swap(campaign.routes, stagedRoutes);
  swap(campaign.survival, stagedSurvival);
  swap(campaign.hearing, stagedHearing);
  swap(campaign.evidence, stagedEvidence);
  swap(campaign.world, stagedWorld);
  return Result::Ok;
}

} // namespace thug::save
