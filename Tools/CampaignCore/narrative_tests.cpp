#include "CampaignCore.h"
#include "CampaignIO.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace thug;
using campaign::ActionKind;
using campaign::Status;
namespace {
int checks=0, occurrences=0;
void Check(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
std::string Bytes(const campaign::CampaignCore& c){std::ostringstream out;Check(c.Save(out)==save::Result::Ok,"Narrative campaign serializes coherently");return out.str();}
director::Observation At(const campaign::CampaignCore& c,const std::string& node){
  director::Observation o;const auto s=c.Narrative();o.difficulty=s.difficulty;o.act=s.act;o.nowSeconds=c.Now();o.joeNode=node;
  for(const auto& name:{"thomas","harley","bartholomeu","jack"})o.companions.emplace(name,director::Actor{node,true});
  return o;
}
campaign::Action Command(const campaign::CampaignCore& c,ActionKind kind,const std::string& target,const std::string& node){
  return {"narrative-"+std::to_string(++occurrences),kind,target,node,c.Now(),0,true};
}
void Apply(campaign::CampaignCore& c,ActionKind kind,const std::string& id,const director::Observation& o){Check(c.ApplyNarrative(Command(c,kind,id,o.joeNode),o).status==Status::Applied,"Verified narrative action confirms");}
campaign::Action Ack(const campaign::CampaignCore& c,const director::Observation& o){
  const auto current=c.CurrentPresentation(o);Check(bool(current),"Current cue exists only in valid context");
  auto action=Command(c,current->owner.rfind("BAN",0)==0?ActionKind::BanterCue:ActionKind::SceneCue,current->owner,o.joeNode);
  action.argument=current->index;action.presentationSerial=current->serial;action.cueId=current->cue.id;return action;
}
void Complete(campaign::CampaignCore& c,director::Observation o){
  while(c.CurrentPresentation(o)){const auto action=Ack(c,o);Check(c.ApplyNarrative(action,o).status==Status::Applied,"Presented line/gesture commits once");o.act=c.Narrative().act;}
}
void Scene(campaign::CampaignCore& c,const std::string& id,const std::string& node){auto o=At(c,node);Apply(c,ActionKind::BeginScene,id,o);Complete(c,o);}
void Opening(campaign::CampaignCore& c){Scene(c,"N01","F4_START");Scene(c,"N02","F4_OBSERVATION");Scene(c,"N03","F4_START");}
void Rejected(campaign::CampaignCore& c,const campaign::Action& a,const director::Observation& o,Status status){const auto before=Bytes(c);Check(c.ApplyNarrative(a,o).status==status,"Rejected narrative reports expected status");Check(Bytes(c)==before,"Rejected narrative changes no state or receipt");}
void ResumeAndReceipts(){
  campaign::CampaignCore c(rules::Difficulty::Hard,5);Scene(c,"N01","F4_START");auto o=At(c,"F4_START");Apply(c,ActionKind::BeginBanter,"BAN01",o);
  const auto first=Ack(c,o);Check(c.ApplyNarrative(first,o).status==Status::Applied,"Banter line confirms");
  Check(c.Narrative().activeBanterLine==1 && c.Narrative().deliveredBanter.empty(),"Partial delivery does not complete conversation");
  const auto checkpoint=Bytes(c);campaign::CampaignCore resumed(rules::Difficulty::Hard,99);std::istringstream in(checkpoint);
  Check(resumed.Load(in)==save::Result::Ok && Bytes(resumed)==checkpoint,"Frozen plan and cursor restore byte-exactly");
  Check(resumed.ApplyNarrative(first,o).status==Status::AlreadyApplied && Bytes(resumed)==checkpoint,"Loaded receipt prevents repeated line effect");
  auto absent=o;absent.companions.clear();Check(!resumed.CurrentPresentation(absent),"Missing actors suspend restored speech");
  auto danger=o;danger.inDanger=true;Rejected(resumed,Ack(resumed,o),danger,Status::MissingRequirement);
  resumed.SetPaused(true);Check(!resumed.CurrentPresentation(o),"Menu pause hides active presentation");
  Rejected(resumed,Ack(c,o),o,Status::Paused);resumed.SetPaused(false);
  auto main=At(resumed,"F4_OBSERVATION");Apply(resumed,ActionKind::BeginScene,"N02",main);
  Check(resumed.Narrative().activeBanterLine==1,"Main scene retains suspended conversation");Complete(resumed,main);
  o=At(resumed,"F4_START");Check(resumed.CurrentPresentation(o)->index==1,"Resume starts at first unconfirmed line");
  Complete(resumed,o);Check(resumed.Narrative().deliveredBanter.count("BAN01"),"All confirmed lines complete conversation");
  Rejected(resumed,Command(resumed,ActionKind::BeginBanter,"BAN01",o.joeNode),o,Status::MissingRequirement);
  campaign::CampaignCore restarted(rules::Difficulty::Hard);o=At(restarted,"F4_START");Apply(restarted,ActionKind::BeginScene,"N01",o);
  const auto old=Ack(restarted,o);const auto view=*restarted.CurrentPresentation(o);auto cancel=Command(restarted,ActionKind::CancelScene,"N01",o.joeNode);cancel.presentationSerial=view.serial;
  Check(restarted.ApplyNarrative(cancel,o).status==Status::Applied,"Cancellation does not mark scene seen");Apply(restarted,ActionKind::BeginScene,"N01",o);
  Rejected(restarted,old,o,Status::MissingRequirement);
  Check(Bytes(resumed).find("THE_THUG_RUNTIME_CAMPAIGN 9")==0,"Runtime uses explicit version for selected plans");
}
void ReadingAndAuthority(){
  campaign::CampaignCore c(rules::Difficulty::Hard);Check(c.JournalText("DOC01").empty(),"Unread text remains unavailable");
  auto o=At(c,"F4_START");Rejected(c,Command(c,ActionKind::InspectDocument,"DOC01",o.joeNode),o,Status::MissingRequirement);
  o=At(c,"F4_NURSE");auto a=Command(c,ActionKind::InspectDocument,"DOC01",o.joeNode);a.physicallyVerified=false;Rejected(c,a,o,Status::InvalidAction);
  Apply(c,ActionKind::InspectDocument,"DOC01",o);Check(!c.JournalText("DOC01").empty() && c.JournalText("DOC02").empty(),"Physical reading updates only its own journal entry");
  Check(c.Narrative().durableFacts.count("code_a") && !c.Narrative().durableFacts.count("code_b"),"Only inspected fragment becomes knowledge");
  o=At(c,"F2_SECURITY");Apply(c,ActionKind::InspectDocument,"DOC05",o);Check(!c.Narrative().durableFacts.count("safe_known"),"Order document cannot invent key discovery");
  o=At(c,"F1_SECURITY");Apply(c,ActionKind::InspectDocument,"DOC04",o);Check(c.Narrative().durableFacts.count("safe_known"),"Previously read order becomes meaningful after actual key information");
  Rejected(c,Command(c,ActionKind::RememberFact,"jack_final_encounter",o.joeNode),o,Status::InvalidAction);
  Rejected(c,Command(c,ActionKind::RememberFact,"garage_escaped",o.joeNode),o,Status::InvalidAction);
  Opening(c);o=At(c,"X_GARAGE");o.facts.insert("garage_escaped");Rejected(c,Command(c,ActionKind::BeginScene,"N28:exit",o.joeNode),o,Status::MissingRequirement);
  campaign::CampaignCore normal(rules::Difficulty::Normal);o=At(normal,"F2_SECURITY");Rejected(normal,Command(normal,ActionKind::InspectDocument,"DOC16",o.joeNode),o,Status::InvalidAction);
  o=At(normal,"F3_EXAMS");o.sightAndSoundVerified=true;Rejected(normal,Command(normal,ActionKind::WitnessClue,"J1",o.joeNode),o,Status::MissingRequirement);
  Check(normal.JournalText("DOC16").empty(),"Forbidden text stays hidden through query API");
}
void CheckpointIntegrity(){
  campaign::CampaignCore core(rules::Difficulty::Hard);auto o=At(core,"F4_START");Apply(core,ActionKind::BeginScene,"N01",o);
  Check(core.ApplyNarrative(Ack(core,o),o).status==Status::Applied,"One cue really completed before checkpoint");
  const auto valid=Bytes(core);
  const auto reject=[&](std::string bytes){
    const auto at=bytes.rfind("CHECKSUM ");Check(at!=std::string::npos,"Fixture has checksum");bytes.resize(at);bytes+="CHECKSUM "+campaign::detail::Hash(bytes)+'\n';
    const auto before=Bytes(core);std::istringstream in(bytes);Check(core.Load(in)==save::Result::IncompatibleRoster,"Semantically incoherent presentation is rejected even with valid checksum");
    Check(Bytes(core)==before,"Presentation rejection changes no campaign state");
  };
  auto bad=valid;const auto serial=bad.find("PRESENTATION 1 1 0");Check(serial!=std::string::npos,"Fixture has active serial");bad.replace(serial,18,"PRESENTATION 2 1 0");reject(bad);
  bad=valid;const auto plan=bad.find("SCENE_PLAN ");const auto cue=bad.find('"',plan);const auto end=bad.find('"',cue+1);bad.replace(cue+1,end-cue-1,"N34:0");reject(bad);
  bad=valid;const auto receipt=bad.find("RECEIPT ");const auto signatureEnd=bad.find('\n',receipt);
  bad.erase(receipt,signatureEnd-receipt+1);const auto count=bad.find("RECEIPTS 2");Check(count!=std::string::npos,"Fixture has begin and acknowledgement receipts");bad.replace(count,10,"RECEIPTS 1");reject(bad);
}
void SecretsAndKeypad(){
  campaign::CampaignCore c(rules::Difficulty::Punitive);Opening(c);
  auto o=At(c,"F3_EXAMS");auto advance=Command(c,ActionKind::AdvanceAct,"act",o.joeNode);advance.argument=2;
  Check(c.ApplyNarrative(advance,o).status==Status::Applied,"Verified milestone advances act without clock shortcut");
  o=At(c,"F3_EXAMS");o.facts={"jack_open_door_observed"};Apply(c,ActionKind::BeginScene,"N08",o);Complete(c,o);
  o=At(c,"F3_EXAMS");o.sightAndSoundVerified=true;Apply(c,ActionKind::WitnessClue,"J1",o);
  o=At(c,"F2_CAMERAS");o.facts={"jack_verified_inference","jack_perceived_companion_risk"};Apply(c,ActionKind::BeginScene,"N25",o);Complete(c,o);
  o=At(c,"F2_CAMERAS");o.facts={"jack_perceived_companion_risk"};o.sightAndSoundVerified=true;Apply(c,ActionKind::WitnessClue,"J3",o);
  advance=Command(c,ActionKind::AdvanceAct,"act",o.joeNode);advance.argument=5;Check(c.ApplyNarrative(advance,o).status==Status::Applied,"Final chapter requires actual milestone");
  o=At(c,"F3_EXAMS");o.sightAndSoundVerified=true;o.alternativeReachable=true;o.facts={"jack_final_encounter"};Apply(c,ActionKind::BeginScene,"J4",o);
  const auto bytes=Bytes(c);campaign::CampaignCore resumed(rules::Difficulty::Punitive);std::istringstream in(bytes);Check(resumed.Load(in)==save::Result::Ok,"Restricted active scene survives permitted-mode load");
  Complete(resumed,o);Check(resumed.Narrative().witnessedClues.count("J4") && resumed.Narrative().seenScenes.count("J4"),"Complete witnessed revelation records both presentation and knowledge");
  Check(!resumed.Narrative().durableFacts.count("jack_final_encounter"),"Encounter flag expires instead of entering journal");Check(!Bytes(resumed).empty(),"Revelation checkpoint remains coherent");
  campaign::CampaignCore keypad(rules::Difficulty::Hard);
  auto wrong=Command(keypad,ActionKind::SubmitKeypad,"000000","F2_SAFE");if(wrong.target==director::Content().garageCode)wrong.target="999999";
  const auto attempted=keypad.Apply(wrong);Check(attempted.status==Status::Applied && attempted.acceptedCode==false && keypad.Narrative().keypadAttempts==1,"Wrong valid input records a real attempt without opening");
  Check(keypad.Apply(wrong).status==Status::AlreadyApplied && keypad.Narrative().keypadAttempts==1,"Retry does not count keypad attempt twice");
  const auto correct=Command(keypad,ActionKind::SubmitKeypad,director::Content().garageCode,"F2_SAFE");const auto accepted=keypad.Apply(correct);
  Check(accepted.status==Status::Applied && accepted.acceptedCode==true && keypad.World().objects.at("SAFE_F2").stateId=="open","Prior knowledge can submit correct input; world and route agree");
  Check(keypad.Narrative().inspectedDocuments.empty() && keypad.Narrative().keypadAttempts==2,"Correct guess never invents inspected clues");
  Check(!Bytes(keypad).empty(),"Keypad attempts and receipts persist together");
}
}
int main(){try{ResumeAndReceipts();ReadingAndAuthority();CheckpointIntegrity();SecretsAndKeypad();std::cout<<"CampaignNarrative: "<<checks<<" checks passed\n";}catch(const std::exception& e){std::cerr<<"After "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
