#include "StorySave.h"

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

constexpr std::size_t MaxBytes = 2 * 1024 * 1024;
constexpr unsigned MaxRecords = 10000;
static_assert(std::is_nothrow_swappable_v<NarrativeState>);

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

bool Tag(std::istream& in, const char* value) {
  std::string received;
  return static_cast<bool>(in >> received) && received == value;
}

bool Quoted(std::istream& in, std::string& value, bool empty = false) {
  in >> std::ws;
  if (in.peek() != '"' || !(in >> std::quoted(value))) return false;
  return value.size() <= 1024 && (empty || !value.empty());
}

bool Count(std::istream& in, unsigned& value) {
  return static_cast<bool>(in >> value) && value <= MaxRecords;
}

bool Bool(std::istream& in, bool& value) {
  int number;
  if (!(in >> number) || (number != 0 && number != 1)) return false;
  value = number != 0;
  return true;
}

void WriteSet(std::ostream& out, const char* heading, const char* item,
              const std::set<std::string>& ids) {
  out << heading << ' ' << ids.size() << '\n';
  for (const auto& id : ids) out << item << ' ' << std::quoted(id) << '\n';
}

bool ReadSet(std::istream& in, const char* heading, const char* item,
             std::set<std::string>& ids) {
  unsigned count;
  if (!Tag(in, heading) || !Count(in, count)) return false;
  for (unsigned i = 0; i < count; ++i) {
    std::string id;
    if (!Tag(in, item) || !Quoted(in, id) ||
        !ids.insert(std::move(id)).second) return false;
  }
  return true;
}

void WriteNarrative(std::ostream& out, const NarrativeSnapshot& s) {
  out << "NARRATIVE " << static_cast<int>(s.difficulty) << ' '
      << s.nowSeconds << ' ' << s.act << ' ' << s.keypadAttempts << ' '
      << s.joeEscaped << ' ' << s.lastBanterCompletedAt << '\n';
  WriteSet(out, "SCENES", "SCENE", s.seenScenes);
  WriteSet(out, "BANTER", "BAN", s.deliveredBanter);
  WriteSet(out, "DOCUMENTS", "DOC", s.inspectedDocuments);
  WriteSet(out, "AUDIOS", "AUD", s.inspectedAudio);
  WriteSet(out, "CLUES", "CLUE", s.witnessedClues);
  WriteSet(out, "FACTS", "FACT", s.durableFacts);
  out << "ACTIVE " << std::quoted(s.activeScene) << ' ' << s.activeSceneLine
      << ' ' << std::quoted(s.activeBanter) << ' ' << s.activeBanterLine << '\n';
  out << "OUTCOMES " << s.outcomes.size() << '\n';
  for (const auto& [name, evidence] : s.outcomes)
    out << "OUTCOME " << std::quoted(name) << ' ' << evidence.together << ' '
        << evidence.escapedAlone << ' ' << evidence.captured << ' '
        << evidence.dead << ' ' << std::quoted(evidence.lastContactNode) << '\n';
}

bool ReadNarrative(std::istream& in, NarrativeSnapshot& s) {
  int difficulty;
  if (!Tag(in, "NARRATIVE") ||
      !(in >> difficulty >> s.nowSeconds >> s.act >> s.keypadAttempts) ||
      !Bool(in, s.joeEscaped) || !(in >> s.lastBanterCompletedAt) ||
      difficulty < 0 || difficulty > 3) return false;
  s.difficulty = static_cast<rules::Difficulty>(difficulty);
  if (!ReadSet(in, "SCENES", "SCENE", s.seenScenes) ||
      !ReadSet(in, "BANTER", "BAN", s.deliveredBanter) ||
      !ReadSet(in, "DOCUMENTS", "DOC", s.inspectedDocuments) ||
      !ReadSet(in, "AUDIOS", "AUD", s.inspectedAudio) ||
      !ReadSet(in, "CLUES", "CLUE", s.witnessedClues) ||
      !ReadSet(in, "FACTS", "FACT", s.durableFacts) ||
      !Tag(in, "ACTIVE") || !Quoted(in, s.activeScene, true) ||
      !(in >> s.activeSceneLine) || !Quoted(in, s.activeBanter, true) ||
      !(in >> s.activeBanterLine)) return false;
  unsigned count;
  if (!Tag(in, "OUTCOMES") || !Count(in, count) || count != 3) return false;
  s.outcomes.clear();
  for (unsigned i = 0; i < count; ++i) {
    std::string name;
    OutcomeEvidence evidence;
    if (!Tag(in, "OUTCOME") || !Quoted(in, name) ||
        !Bool(in, evidence.together) || !Bool(in, evidence.escapedAlone) ||
        !Bool(in, evidence.captured) || !Bool(in, evidence.dead) ||
        !Quoted(in, evidence.lastContactNode, true) ||
        !s.outcomes.emplace(std::move(name), std::move(evidence)).second)
      return false;
  }
  return true;
}

Result ReadBounded(std::istream& input, std::string& bytes) {
  char byte;
  while (input.get(byte)) {
    if (bytes.size() == MaxBytes) return Result::TooLarge;
    bytes.push_back(byte);
  }
  return input.eof() && !input.bad() ? Result::Ok : Result::IoError;
}

bool Consistent(const StoryCampaign& campaign) {
  const auto story = campaign.narrative.CaptureSnapshot();
  const auto world = campaign.core.world.CaptureSnapshot();
  const auto ending = campaign.core.routes.Result();
  const bool routeEscape = ending == rules::Ending::GarageEscape ||
      ending == rules::Ending::RoofEscape ||
      ending == rules::Ending::TunnelEscape ||
      ending == rules::Ending::WindowEscape || ending == rules::Ending::CoordinatedEscape;
  const std::pair<const char*, rules::Ending> codas[] = {
      {"N28:exit", rules::Ending::GarageEscape},
      {"N29:exit", rules::Ending::RoofEscape},
      {"N30:exit", rules::Ending::TunnelEscape},
      {"N31:exit", rules::Ending::WindowEscape}};
  for (const auto& [id, expected] : codas)
    if ((story.activeScene == id || story.seenScenes.count(id)) && ending != expected)
      return false;
  if ((story.activeScene == "N33" || story.seenScenes.count("N33")) &&
      ending != rules::Ending::CoordinatedEscape && ending != rules::Ending::BitterDestruction) return false;
  return story.difficulty == campaign.core.routes.DifficultyLevel() &&
         std::isfinite(world.nowSeconds) && story.nowSeconds <= world.nowSeconds &&
         (!story.joeEscaped || routeEscape);
}

} // namespace

Result SaveStoryCampaign(std::ostream& output, const StoryCampaign& campaign) {
  if (!Consistent(campaign)) return Result::IncompatibleRoster;
  std::ostringstream core;
  const auto result = SaveCampaign(core, campaign.core);
  if (result != Result::Ok) return result;
  const auto bytes = core.str();
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10)
      << "THE_THUG_STORY_CAMPAIGN 1\nCORE " << bytes.size() << '\n'
      << bytes;
  WriteNarrative(out, campaign.narrative.CaptureSnapshot());
  out << "END\n";
  const auto payload = out.str();
  const auto data = payload + "CHECKSUM " + Checksum(payload) + '\n';
  if (data.size() > MaxBytes) return Result::TooLarge;
  output.write(data.data(), static_cast<std::streamsize>(data.size()));
  return output ? Result::Ok : Result::IoError;
}

Result LoadStoryCampaign(std::istream& input, StoryCampaign campaign) {
  if (campaign.narrative.DifficultyLevel() !=
      campaign.core.routes.DifficultyLevel()) return Result::IncompatibleRoster;
  std::string data;
  const auto readResult = ReadBounded(input, data);
  if (readResult != Result::Ok) return readResult;
  std::istringstream in(data);
  in.imbue(std::locale::classic());
  int version;
  if (!Tag(in, "THE_THUG_STORY_CAMPAIGN") || !(in >> version))
    return Result::Malformed;
  if (version != 1) return Result::UnsupportedVersion;
  std::size_t length;
  if (!Tag(in, "CORE") || !(in >> length) || length > MaxBytes ||
      in.get() != '\n' || length > data.size()) return Result::Malformed;
  std::string core(length, '\0');
  if (!in.read(core.data(), static_cast<std::streamsize>(length)))
    return Result::Malformed;
  NarrativeSnapshot story;
  if (!ReadNarrative(in, story) || !Tag(in, "END")) return Result::Malformed;
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

  auto joe = campaign.core.party.joe;
  auto harley = campaign.core.party.harley;
  auto bartholomeu = campaign.core.party.bartholomeu;
  auto jack = campaign.core.party.jack;
  auto hunter = campaign.core.hunter;
  auto routes = campaign.core.routes;
  auto survival = campaign.core.survival;
  auto hearing = campaign.core.hearing;
  auto evidence = campaign.core.evidence;
  auto world = campaign.core.world;
  auto narrative = campaign.narrative;
  Campaign staged{{joe, harley, bartholomeu, jack}, hunter, routes, survival,
                  hearing, evidence, world};
  std::istringstream coreIn(core);
  const auto coreResult = LoadCampaign(coreIn, staged);
  if (coreResult != Result::Ok) return coreResult;
  StoryCampaign stagedStory{staged, narrative};
  if (!narrative.RestoreSnapshot(story) || !Consistent(stagedStory))
    return Result::IncompatibleRoster;

  using std::swap;
  swap(campaign.core.party.joe, joe);
  swap(campaign.core.party.harley, harley);
  swap(campaign.core.party.bartholomeu, bartholomeu);
  swap(campaign.core.party.jack, jack);
  swap(campaign.core.hunter, hunter);
  swap(campaign.core.routes, routes);
  swap(campaign.core.survival, survival);
  swap(campaign.core.hearing, hearing);
  swap(campaign.core.evidence, evidence);
  swap(campaign.core.world, world);
  swap(campaign.narrative, narrative);
  return Result::Ok;
}

} // namespace thug::save
