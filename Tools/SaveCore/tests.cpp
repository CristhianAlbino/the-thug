#include "SaveCore.h"
#include "../IntegrationCore/IntegrationCore.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace thug;

namespace {

int checks = 0;

void Check(bool value, const char* reason) {
  ++checks;
  if (!value) throw std::runtime_error(reason);
}

std::vector<survival::ItemDefinition> Catalog() {
  return {{"magazine", survival::ItemCategory::Ammunition, 1, 5},
          {"bandage", survival::ItemCategory::Medical, 1, 3}};
}

companion::Traits Neutral() { return {50, 50, 50, 50, 50, 50, 0, false}; }

struct Fixture {
  survival::Inventory joe{6, Catalog()};
  companion::CompanionCore harley{companion::Character::Harley,
                                   companion::Difficulty::Hard, Neutral(), 11};
  companion::CompanionCore barth{companion::Character::Bartholomeu,
                                 companion::Difficulty::Hard, Neutral(), 22};
  companion::CompanionCore jack{companion::Character::Jack,
                                companion::Difficulty::Hard, Neutral(), 33};

  save::Party Members() { return {joe, harley, barth, jack}; }
};

std::string Encode(save::Party party) {
  std::ostringstream out;
  Check(save::SaveParty(out, party) == save::Result::Ok, "Save succeeds");
  return out.str();
}

save::Result Decode(const std::string& data, save::Party party) {
  std::istringstream in(data);
  return save::LoadParty(in, party);
}

void RoundTripAndReplay() {
  Fixture original;
  Check(original.joe.Add("magazine", 3) &&
            original.joe.Add("bandage", 2), "Joe has authored supplies");
  Check(integration::ShareAmmunition(original.joe, "magazine",
                                     original.harley, "handoff-1", 5),
        "Handoff before saving");
  Check(original.harley.LearnRoute("roof", 6) &&
            original.harley.LearnRoute("roof \"north\"", 6) &&
            original.barth.LearnRoute("tunnel", 7),
        "Knowledge is per companion");
  Check(original.jack.ApplyExperience(
            companion::Experience::JoeAbandonedCompanion, "abandoned-1", 8),
        "Jack remembers abandonment");
  original.barth.SetInjured(true, 9);
  companion::Situation situation;
  situation.joePerceivedNearby = true;
  situation.joeNeedsHelp = true;
  situation.canReachJoe = true;
  for (int i = 0; i < 5; ++i) original.harley.Choose(situation);

  const auto saved = Encode(original.Members());
  Check(saved.rfind("THE_THUG_PARTY 1\n", 0) == 0, "Save has version header");
  Fixture loaded;
  Check(loaded.joe.Add("magazine", 5), "Loaded fixture initially differs");
  loaded.harley.SetInjured(true, 100);
  Check(Decode(saved, loaded.Members()) == save::Result::Ok,
        "Earlier game time can be restored");
  Check(Encode(loaded.Members()) == saved,
        "Four states and RNG round trip exactly");
  Check(loaded.joe.Quantity("magazine") == 2 &&
            loaded.joe.Quantity("bandage") == 2 &&
            loaded.harley.CurrentTraits().ammunition == 1,
        "Joe and Harley retain one real transfer");
  Check(loaded.harley.KnownRoutes().count("roof") &&
            loaded.harley.KnownRoutes().count("roof \"north\"") &&
            !loaded.jack.KnownRoutes().count("roof") &&
            loaded.barth.KnownRoutes().count("tunnel"),
        "Individual route knowledge persists");
  Check(loaded.barth.CurrentTraits().injured &&
            loaded.jack.CurrentTraits().trust == 10,
        "Injury and abandonment effects persist");
  Check(!integration::ShareAmmunition(loaded.joe, "magazine",
                                      loaded.harley, "handoff-1", 10),
        "Replayed handoff rejected after load");
  Check(loaded.joe.Quantity("magazine") == 2 &&
            loaded.harley.CurrentTraits().ammunition == 1,
        "Replay changes neither side");
  Check(!loaded.jack.ApplyExperience(
            companion::Experience::JoeAbandonedCompanion, "abandoned-1", 10),
        "Abandonment ID persists");
  for (int i = 0; i < 25; ++i) {
    Check(original.harley.Choose(situation).goal ==
              loaded.harley.Choose(situation).goal,
          "RNG continues with same choices after load");
  }
  Check(integration::ShareAmmunition(loaded.joe, "magazine",
                                     loaded.harley, "handoff-2", 10),
        "New event after load still succeeds");
  Check(loaded.joe.Quantity("magazine") == 1 &&
            loaded.harley.CurrentTraits().ammunition == 2,
        "New event conserves ammo");
}

void RejectedInputIsAtomic() {
  Fixture fixture;
  Check(fixture.joe.Add("magazine", 2), "Fixture stock ready");
  const auto clean = Encode(fixture.Members());
  const auto originalState = clean;
  const auto Reject = [&](const std::string& data, save::Result reason) {
    Check(Decode(data, fixture.Members()) == reason,
          "Invalid input returns expected error");
    Check(Encode(fixture.Members()) == originalState,
          "Failed load leaves all four objects unchanged");
  };

  Reject(clean.substr(0, clean.size() - 8), save::Result::Malformed);
  auto changed = clean;
  changed.replace(0, std::string("THE_THUG_PARTY").size(), "WRONG_FORMAT__");
  Reject(changed, save::Result::Malformed);
  changed = clean;
  changed.replace(0, std::string("THE_THUG_PARTY 1").size(),
                  "THE_THUG_PARTY 2");
  Reject(changed, save::Result::UnsupportedVersion);
  Reject(clean + "INJECTED", save::Result::Malformed);
  Reject(std::string(2 * 1024 * 1024 + 1, 'X'), save::Result::TooLarge);

  changed = clean;
  const auto rng = changed.find("RNG \"");
  Check(rng != std::string::npos, "RNG field present");
  changed[rng + 5] = 'x';
  Reject(changed, save::Result::Corrupt);

  changed = clean;
  const auto trust = changed.find("TRAITS 50 ");
  Check(trust != std::string::npos, "Traits present");
  changed.replace(trust, std::string("TRAITS 50 ").size(), "TRAITS 101 ");
  Reject(changed, save::Result::Corrupt);

  changed = clean;
  const auto stock = changed.find("STOCK \"magazine\" 2");
  Check(stock != std::string::npos, "Stock present");
  changed.replace(stock, std::string("STOCK \"magazine\" 2").size(),
                  "STOCK \"magazine\" 999");
  Reject(changed, save::Result::Corrupt);

  changed = clean;
  changed[changed.find("CHECKSUM ") + 9] = '0';
  if (changed == clean) changed[changed.find("CHECKSUM ") + 9] = '1';
  Reject(changed, save::Result::Corrupt);

  Check(fixture.harley.LearnRoute(std::string(1025, 'A'), 1),
        "Long route can exist in memory");
  std::ostringstream oversized;
  Check(save::SaveParty(oversized, fixture.Members()) == save::Result::TooLarge,
        "Never writes a save the parser would reject");
  Check(oversized.str().empty(), "Oversized ID writes no bytes");
}

void RejectCatalogAndRosterMismatch() {
  Fixture original;
  Check(original.joe.Add("magazine", 1), "Stock for catalog check");
  const auto saved = Encode(original.Members());

  survival::Inventory changedCatalog(6,
      {{"magazine", survival::ItemCategory::Ammunition, 2, 5},
       {"bandage", survival::ItemCategory::Medical, 1, 3}});
  save::Party target{changedCatalog, original.harley, original.barth,
                     original.jack};
  Check(Decode(saved, target) == save::Result::IncompatibleRoster,
        "Changed item cost rejected");
  Check(changedCatalog.Contents().empty(),
        "Catalog mismatch leaves target unchanged");

  save::Party wrong{original.joe, original.barth, original.harley,
                    original.jack};
  std::ostringstream out;
  Check(save::SaveParty(out, wrong) == save::Result::IncompatibleRoster,
        "Cannot save swapped identities");
  Check(out.str().empty(), "Rejected save writes nothing");
  Check(Decode(saved, wrong) == save::Result::IncompatibleRoster,
        "Cannot load into swapped identities");
}

} // namespace

int main() {
  try {
    RoundTripAndReplay();
    RejectedInputIsAtomic();
    RejectCatalogAndRosterMismatch();
    std::cout << "SaveCore: " << checks << " checks passed\n";
  } catch (const std::exception& error) {
    std::cerr << "SaveCore test failed after " << checks
              << " checks: " << error.what() << '\n';
    return 1;
  }
}
