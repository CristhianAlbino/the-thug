#include "CampaignState.h"
#include "CampaignIO.h"

namespace thug::campaign {
namespace {
bool Tag(std::istream& in, const char* expected) {
  std::string received;
  return bool(in >> received) && received == expected;
}

bool Quoted(std::istream& in, std::string& value, std::size_t maximum, bool empty = false) {
  in >> std::ws;
  return in.peek() == '"' && bool(in >> std::quoted(value)) &&
         value.size() <= maximum && (empty || !value.empty());
}
void WritePlan(std::ostream& out, const char* tag, const std::vector<std::string>& plan) {
  out << tag << ' ' << plan.size() << '\n';
  for (const auto& cue : plan) out << "CUE " << std::quoted(cue) << '\n';
}
bool ReadPlan(std::istream& in, const char* tag, std::vector<std::string>& plan) {
  std::size_t count;
  if (!Tag(in, tag) || !(in >> count) || count > 1000) return false;
  for (std::size_t i = 0; i < count; ++i) {
    std::string cue;
    if (!Tag(in, "CUE") || !Quoted(in, cue, 128)) return false;
    plan.push_back(std::move(cue));
  }
  return true;
}
} // namespace

save::Result CampaignCore::Save(std::ostream& output) const {
  if (!Consistent(*state_)) return save::Result::IncompatibleRoster;
  std::ostringstream story;
  const auto result = save::SaveStoryCampaign(story, state_->Members());
  if (result != save::Result::Ok) return result;
  const auto nested = story.str();
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << "THE_THUG_RUNTIME_CAMPAIGN 8\nCONTENT " << contentIdentity_
      << "\nPAUSED " << state_->paused << "\nHIDE " << std::quoted(state_->hidingPlace)
      << "\nSTORY " << nested.size() << '\n' << nested
      << "PRESENTATION " << state_->presentation.serial << ' ' << state_->presentation.sceneSerial
      << ' ' << state_->presentation.banterSerial << '\n';
  WritePlan(out, "SCENE_PLAN", state_->presentation.scenePlan);
  WritePlan(out, "BANTER_PLAN", state_->presentation.banterPlan);
  WriteCompanions(out, *state_);
  WriteHunter(out, *state_);
  WriteCombat(out, *state_);
  WriteSpecial(out, *state_);
  WriteInteractions(out, *state_);
  WriteRescue(out, *state_);
  out << "RECEIPTS " << state_->receipts.size() << '\n';
  for (const auto& [id, signature] : state_->receipts)
    out << "RECEIPT " << std::quoted(id) << ' ' << std::quoted(signature) << '\n';
  out << "END\n";
  const auto payload = out.str();
  const auto bytes = payload + "CHECKSUM " + detail::Hash(payload) + '\n';
  if (bytes.size() > detail::MaxRuntimeBytes) return save::Result::TooLarge;
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return output ? save::Result::Ok : save::Result::IoError;
}

save::Result CampaignCore::Load(std::istream& input) {
  std::string bytes;
  char byte;
  while (input.get(byte)) {
    if (bytes.size() == detail::MaxRuntimeBytes) return save::Result::TooLarge;
    bytes.push_back(byte);
  }
  if (!input.eof() || input.bad()) return save::Result::IoError;
  std::istringstream in(bytes);
  in.imbue(std::locale::classic());
  int version;
  if (!Tag(in, "THE_THUG_RUNTIME_CAMPAIGN") || !(in >> version)) return save::Result::Malformed;
  if (version != 8) return save::Result::UnsupportedVersion;
  std::string identity, hide;
  int paused;
  std::size_t length, count;
  if (!Tag(in, "CONTENT") || !(in >> identity) || identity.size() != 16 ||
      !Tag(in, "PAUSED") || !(in >> paused) || (paused != 0 && paused != 1) ||
      !Tag(in, "HIDE") || !Quoted(in, hide, 128, true) ||
      !Tag(in, "STORY") || !(in >> length) || in.get() != '\n' ||
      length > detail::MaxRuntimeBytes || length > bytes.size()) return save::Result::Malformed;
  std::string story(length, '\0');
  auto next = std::make_unique<State>(*state_);
  director::PresentationState presentation;
  if (!in.read(story.data(), static_cast<std::streamsize>(length)) ||
      !Tag(in, "PRESENTATION") || !(in >> presentation.serial >> presentation.sceneSerial >> presentation.banterSerial) ||
      !ReadPlan(in, "SCENE_PLAN", presentation.scenePlan) || !ReadPlan(in, "BANTER_PLAN", presentation.banterPlan) || !ReadCompanions(in, *next) || !ReadHunter(in, *next) || !ReadCombat(in, *next) || !ReadSpecial(in, *next) ||
      !ReadInteractions(in, *next) || !ReadRescue(in, *next) || !Tag(in, "RECEIPTS") || !(in >> count) || count > detail::MaxReceipts) return save::Result::Malformed;
  std::map<std::string, std::string> receipts;
  for (std::size_t i = 0; i < count; ++i) {
    std::string id, signature;
    if (!Tag(in, "RECEIPT") || !Quoted(in, id, 128) ||
        !Quoted(in, signature, 4096) || !receipts.emplace(id, signature).second) return save::Result::Malformed;
  }
  std::string rest, checksum;
  if (!Tag(in, "END") || !std::getline(in, rest) || !rest.empty()) return save::Result::Malformed;
  const auto checksumPosition = in.tellg();
  if (checksumPosition < 0 || !Tag(in, "CHECKSUM") || !(in >> checksum) || checksum.size() != 16)
    return save::Result::Malformed;
  in >> std::ws;
  if (!in.eof()) return save::Result::Malformed;
  if (checksum != detail::Hash(bytes.substr(0, static_cast<std::size_t>(checksumPosition)))) return save::Result::Corrupt;
  if (identity != contentIdentity_) return save::Result::IncompatibleRoster;
  std::istringstream nested(story);
  const auto result = save::LoadStoryCampaign(nested, next->Members());
  if (result != save::Result::Ok) return result;
  next->paused = paused != 0;
  next->hidingPlace = std::move(hide);
  next->receipts = std::move(receipts);
  next->presentation = std::move(presentation);
  if (!RestoreInteractions(*next)) return save::Result::IncompatibleRoster;
  if (!Consistent(*next)) return save::Result::IncompatibleRoster;
  state_.swap(next);
  return save::Result::Ok;
}
save::Result CampaignCore::ValidateCheckpoint(std::istream& input) const {
  CampaignCore candidate(state_->routes.DifficultyLevel(), *definition_, 0, state_->survival.CaptureSnapshot().tuning);
  return candidate.Load(input);
}
} // namespace thug::campaign
