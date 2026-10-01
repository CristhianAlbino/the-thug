#pragma once

// Private implementation shared by CampaignCore.cpp and CampaignSave.cpp.
#include "CampaignCore.h"
#include "CampaignData.generated.h"

namespace thug::campaign {

struct CampaignCore::State {
  State(rules::Difficulty difficulty, const WorldDefinition& world,
        std::uint64_t seed, survival::Tuning tuning)
      : profile(data::For(difficulty)), inventory(profile.capacity, data::Items()),
        harley(companion::Character::Harley, static_cast<companion::Difficulty>(difficulty), profile.companions[0], seed + 1),
        bartholomeu(companion::Character::Bartholomeu, static_cast<companion::Difficulty>(difficulty), profile.companions[1], seed + 2),
        jack(companion::Character::Jack, static_cast<companion::Difficulty>(difficulty), profile.companions[2], seed + 3),
        hunter(static_cast<hunter::Difficulty>(difficulty), seed + 4), routes(difficulty),
        survival(tuning), hearing({}, seed + 5), world(world.levelId, world.objects),
        narrative(difficulty) {
    for (const auto& [id, count] : profile.loadout)
      if (!inventory.Add(id, count)) throw std::logic_error("Invalid authored loadout");
  }

  save::StoryCampaign Members() {
    return {{{inventory, harley, bartholomeu, jack}, hunter, routes, survival,
             hearing, evidence, world}, narrative};
  }

  companion::CompanionCore& Person(companion::Character person) {
    switch (person) {
      case companion::Character::Harley: return harley;
      case companion::Character::Bartholomeu: return bartholomeu;
      case companion::Character::Jack: return jack;
    }
    throw std::invalid_argument("Unknown companion");
  }

  const companion::CompanionCore& Person(companion::Character person) const {
    switch (person) {
      case companion::Character::Harley: return harley;
      case companion::Character::Bartholomeu: return bartholomeu;
      case companion::Character::Jack: return jack;
    }
    throw std::invalid_argument("Unknown companion");
  }

  void AdvanceClocks(double now) {
    hunter.AdvanceTo(now);
    routes.AdvanceTo(now);
    harley.AdvanceTo(now);
    bartholomeu.AdvanceTo(now);
    jack.AdvanceTo(now);
    if (!world.AdvanceTo(now)) throw std::logic_error("Invalid shared world clock");
    auto story = narrative.CaptureSnapshot();
    story.nowSeconds = now;
    if (!narrative.RestoreSnapshot(story)) throw std::logic_error("Invalid narrative clock");
  }

  void SyncEnding() {
    auto story = narrative.CaptureSnapshot();
    const auto ending = routes.Result();
    story.joeEscaped = ending == rules::Ending::GarageEscape || ending == rules::Ending::RoofEscape ||
                       ending == rules::Ending::TunnelEscape || ending == rules::Ending::WindowEscape;
    if (!narrative.RestoreSnapshot(story)) throw std::logic_error("Invalid narrated ending");
  }

  data::Profile profile;
  survival::Inventory inventory;
  companion::CompanionCore harley, bartholomeu, jack;
  hunter::HunterCore hunter;
  rules::RunRules routes;
  survival::SurvivalCore survival;
  perception::PerceptionCore hearing;
  perception::EvidenceMemory evidence;
  save::WorldState world;
  save::NarrativeState narrative;
  bool paused = false;
  std::string hidingPlace;
  std::map<std::string, std::string> receipts;
};

} // namespace thug::campaign
