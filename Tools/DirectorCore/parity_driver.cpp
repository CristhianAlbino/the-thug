#include "DirectorCore.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace thug;
namespace {
void ReadSet(std::istream& in, std::set<std::string>& values) {
  unsigned count; if (!(in >> count) || count > 1000) throw std::runtime_error("bad vector");
  for (unsigned i=0; i<count; ++i) { std::string id; if (!(in >> std::quoted(id))) throw std::runtime_error("bad ID"); values.insert(id); }
}
}
int main() {
  try {
    std::string line;
    while (std::getline(std::cin,line)) {
      std::istringstream in(line);
      std::string kind,id;
      int mode;
      director::Observation o;
      save::NarrativeSnapshot s;
      if (!(in >> kind >> std::quoted(id) >> mode >> o.act >> std::quoted(o.joeNode) >> o.nowSeconds
          >> o.hunterHasEvidence >> o.hunterCanReach >> o.alternativeReachable >> o.areaPreviouslyVisited
          >> std::quoted(o.routeFailure) >> o.inDanger >> o.sightAndSoundVerified
          >> std::quoted(s.activeScene) >> std::quoted(s.activeBanter) >> s.lastBanterCompletedAt)) throw std::runtime_error("bad context");
      o.difficulty = s.difficulty = static_cast<rules::Difficulty>(mode); s.act = o.act;
      ReadSet(in,s.seenScenes); ReadSet(in,s.deliveredBanter); ReadSet(in,s.witnessedClues); ReadSet(in,s.durableFacts); ReadSet(in,o.facts);
      unsigned actors; in >> actors;
      for (unsigned i=0; i<actors; ++i) {std::string name; director::Actor actor; in >> std::quoted(name) >> std::quoted(actor.node) >> actor.active; o.companions.emplace(name,actor);}
      if (kind == "epilogue") {
        in >> s.joeEscaped;
        for (auto& [name,evidence] : s.outcomes) { (void)name; in >> evidence.together >> evidence.escapedAlone >> evidence.captured >> evidence.dead; }
        const auto result=director::ResolveEpilogue(s);
        for(const auto& name : {"bartholomeu","harley","jack","jack_secret","thomas"}) std::cout << std::quoted(result.at(name)) << ' ';
        std::cout << '\n';
      } else {
        const bool result=kind == "scene" ? director::SceneEligible(id,o,s) : kind == "banter" ? director::BanterEligible(id,o,s) :
          kind == "resume" ? director::BanterCanResume(o,s) : kind == "clue" ? director::CanWitness(id,o,s) :
          kind == "variant" ? director::VariantAvailable(id,o.difficulty,s.witnessedClues,o.act) : director::InspectAllowed(id,o.difficulty);
        std::cout << result << '\n';
      }
      if (!in) throw std::runtime_error("truncated vector");
    }
  } catch(const std::exception& e) {std::cerr << e.what() << '\n'; return 1;}
}
