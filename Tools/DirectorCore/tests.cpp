#include "DirectorCore.h"
#include <iostream>
#include <stdexcept>
using namespace thug;
namespace {
int checks=0;
void Check(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
director::Observation At(rules::Difficulty mode,int act,const std::string& node){
  director::Observation o; o.difficulty=mode;o.act=act;o.joeNode=node;
  for(const auto& name:{"thomas","harley","bartholomeu","jack"})o.companions.emplace(name,director::Actor{node,true});
  return o;
}
void Complete(director::Observation& o,save::NarrativeSnapshot& s,director::PresentationState& p){
  while(const auto current=director::Current(o,s,p)){
    Check(director::Acknowledge(*current,o,s,p),"Verified current cue advances");
    o.act=s.act;
  }
}
void Interruptions(){
  auto o=At(rules::Difficulty::Hard,1,"F4_START");save::NarrativeSnapshot s;s.difficulty=o.difficulty;s.seenScenes.insert("N01");director::PresentationState p;
  Check(director::BeginBanter("BAN01",o,s,p),"Optional conversation starts with actual actors");
  auto first=*director::Current(o,s,p);Check(director::Acknowledge(first,o,s,p),"First line confirms independently");
  Check(s.activeBanterLine==1 && !s.deliveredBanter.count("BAN01"),"Partial conversation is not completed");
  auto danger=o;danger.inDanger=true;Check(!director::Current(danger,s,p),"Danger suspends optional speech");
  o.joeNode="F4_OBSERVATION";for(auto& [name,actor]:o.companions){(void)name;actor.node=o.joeNode;}
  Check(director::BeginScene("N02",o,s,p),"Main scene preempts optional conversation");
  Check(s.activeBanterLine==1 && !director::BanterCanResume(o,s),"Suspended conversation retains cursor");
  Complete(o,s,p);Check(s.seenScenes.count("N02") && s.durableFacts.count("thomas_promised_coffee"),"Presented scene records exact observed content");
  o.joeNode="F4_START";for(auto& [name,actor]:o.companions){(void)name;actor.node=o.joeNode;}
  Check(director::BanterCanResume(o,s),"Optional speech resumes after main scene at valid place");
  Check(!director::Acknowledge(first,o,s,p),"Stale line ACK cannot skip next line");
  Complete(o,s,p);Check(s.deliveredBanter.count("BAN01"),"Conversation completes only after all lines");
  s.deliveredBanter.clear();Check(!director::BanterEligible("BAN01",o,s),"Cooldown blocks another occurrence");o.nowSeconds=90;
  Check(director::BeginBanter("BAN01",o,s,p),"Cooldown expires at defined boundary");
  auto current=*director::Current(o,s,p);Check(director::CancelBanter(s,p),"Cancel drops intent without completion");
  Check(director::BeginBanter("BAN01",o,s,p),"Interrupted conversation may be authored again");
  Check(!director::Acknowledge(current,o,s,p),"Activation serial rejects ACK from previous presentation");
}
void ContentAndSecrets(){
  auto o=At(rules::Difficulty::Hard,2,"F1_SECURITY");save::NarrativeSnapshot s;s.difficulty=o.difficulty;s.act=2;s.seenScenes.insert("N03");director::PresentationState p;
  Check(!director::BeginScene("N13",o,s,p),"Unread document cannot produce informed voiceover");
  s.durableFacts.insert("doc04_read");Check(director::BeginScene("N13",o,s,p),"Actual reading enables informed text");
  Check(!director::Current(o,s,p,"en"),"Untranslated locale does not invent translation");
  auto invalid=p;invalid.scenePlan[0]="N34:0";Check(!director::ValidPresentation(s,invalid),"Different scene cue cannot load into presentation");
  Check(director::InspectedText("DOC01",s).empty(),"Journal never grants an uninspected document");
  s.inspectedDocuments.insert("DOC02");Check(director::JournalFragments(s).size()==1,"Journal reveals only inspected fragment");
  Check(!director::InspectAllowed("DOC16",rules::Difficulty::Normal) && !director::InspectAllowed("AUD07",rules::Difficulty::Hard),"Restricted material follows difficulty");
  Check(!director::PersistentFact("jack_final_encounter"),"Current encounter never becomes durable knowledge");
  o=At(rules::Difficulty::Punitive,5,"F3_EXAMS");o.sightAndSoundVerified=true;o.alternativeReachable=true;o.facts={"jack_final_encounter"};
  s={};s.difficulty=o.difficulty;s.act=5;s.witnessedClues={"J1","J3"};p={};
  Check(director::BeginScene("J4",o,s,p),"Secret requires witnessed clues and physical encounter");
  Complete(o,s,p);Check(s.witnessedClues.count("J4") && s.seenScenes.count("J4"),"Presented revelation and witnessed identity record together");
  o.difficulty=rules::Difficulty::Hard;s.difficulty=o.difficulty;Check(!director::SceneEligible("J4",o,s),"Secret never leaks into common difficulty");
  s={};s.difficulty=rules::Difficulty::Hard;
  const auto epilogue=director::ResolveEpilogue(s);Check(epilogue.at("thomas")=="E_THOMAS_CODA_PLAYER_ONLY" && epilogue.at("harley")=="E_HARLEY_UNKNOWN","Unverified companion fate remains unknown");
  s.outcomes.at("harley").dead=true;s.outcomes.at("harley").captured=true;Check(director::ResolveEpilogue(s).empty(),"Conflicting outcome evidence is rejected");
}
}
int main(){try{Interruptions();ContentAndSecrets();std::cout<<"DirectorCore: "<<checks<<" checks passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
