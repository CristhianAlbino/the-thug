#include "CheckpointFile.h"
#include "CampaignIO.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace thug;
namespace fs=std::filesystem;
namespace disk=campaign::checkpoint;
namespace {
int checks=0;
void Check(bool value,const char* reason){++checks;if(!value)throw std::runtime_error(reason);}
struct Folder {
  fs::path path=fs::temp_directory_path()/fs::path("thug-checkpoint-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  Folder(){fs::create_directory(path);}
  ~Folder(){std::error_code e;fs::remove_all(path,e);}
};
std::string Bytes(const campaign::CampaignCore& core){std::ostringstream out;Check(core.Save(out)==save::Result::Ok,"Coherent core serializes");return out.str();}
void Write(const fs::path& path,const std::string& bytes){std::ofstream out(path,std::ios::binary);out<<bytes;Check(bool(out),"Fixture writes");}
std::string Read(const fs::path& path){std::ifstream in(path,std::ios::binary);std::ostringstream out;out<<in.rdbuf();return out.str();}
void Advance(campaign::CampaignCore& core,double dt){Check(core.Tick(dt,survival::Activity::Rest).status==campaign::Status::Applied,"Fixture advances simulation");}
void Recovery(){
  Folder f;const auto slot=f.path/fs::u8path("sess\u00e3o-\u6e2c\u8a66.sav");const auto backup=fs::path(slot.native()+fs::path(".bak").native());
  campaign::CampaignCore core(rules::Difficulty::Hard,1);Advance(core,1);const auto first=Bytes(core);
  Check(disk::SaveFile(core,slot).status==disk::Status::Ok,"Unicode slot saves");Advance(core,8);const auto latest=Bytes(core);
  Check(disk::SaveFile(core,slot).status==disk::Status::Ok && Read(backup)==first,"Previous validated checkpoint becomes backup");
  campaign::CampaignCore loaded(rules::Difficulty::Hard,999);Check(disk::LoadFile(loaded,slot).status==disk::Status::Ok && Bytes(loaded)==latest,"Primary restores exact campaign");
  Write(slot,"truncated");Check(disk::LoadFile(loaded,slot).status==disk::Status::RecoveredBackup && Bytes(loaded)==first,"Corruption explicitly recovers previous coherent state");
  const auto before=Bytes(loaded);Write(backup,"also broken");Check(disk::LoadFile(loaded,slot).status==disk::Status::InvalidCheckpoint && Bytes(loaded)==before,"Two damaged files never mutate live campaign");
  Write(backup,first);fs::remove(slot);Check(disk::LoadFile(loaded,slot).status==disk::Status::RecoveredBackup,"Missing primary can recover");
  Write(slot,std::string(campaign::detail::MaxRuntimeBytes+1,'x'));Check(disk::LoadFile(loaded,slot).status==disk::Status::RecoveredBackup,"Oversized primary is bounded and recovers");
  Check(disk::SaveFile(core,slot).status==disk::Status::Ok && Read(backup)==first,"Saving over damage preserves valid backup");
  auto future=latest;future.replace(0,std::string("THE_THUG_RUNTIME_CAMPAIGN 4").size(),"THE_THUG_RUNTIME_CAMPAIGN 5");Write(slot,future);
  const auto unchanged=Bytes(loaded);auto result=disk::LoadFile(loaded,slot);
  Check(result.status==disk::Status::InvalidCheckpoint && result.checkpoint==save::Result::UnsupportedVersion && Bytes(loaded)==unchanged,"Future version cannot silently roll back progress");
  Check(disk::SaveFile(core,slot).status==disk::Status::InvalidCheckpoint && Read(slot)==future,"Future primary is never overwritten");
  Write(slot,latest);campaign::CampaignCore incompatible(rules::Difficulty::Normal);
  result=disk::LoadFile(incompatible,slot);Check(result.status==disk::Status::InvalidCheckpoint && result.checkpoint==save::Result::IncompatibleRoster,"Incompatible mode cannot fallback");
  Check(disk::SaveFile(incompatible,slot).status==disk::Status::InvalidCheckpoint && Read(slot)==latest,"Incompatible content is not overwritten");
}
void Interruptions(){
  for(const auto stage:{disk::Stage::TempCreated,disk::Stage::PayloadWritten,disk::Stage::PayloadSynced,disk::Stage::BackupSynced,disk::Stage::PrimaryReplaced,disk::Stage::DirectorySynced}){
    Folder f;const auto slot=f.path/"slot.sav";campaign::CampaignCore old(rules::Difficulty::Hard);Advance(old,1);
    Check(disk::SaveFile(old,slot).status==disk::Status::Ok,"Initial primary saves");const auto original=Read(slot);
    campaign::CampaignCore next(rules::Difficulty::Hard);Advance(next,9);
    const auto result=disk::SaveFile(next,slot,[stage](disk::Stage current){return current!=stage;});
    const bool replaced=stage>=disk::Stage::PrimaryReplaced;
    Check(result.status==(stage==disk::Stage::DirectorySynced?disk::Status::Ok:replaced?disk::Status::CommittedUnconfirmed:disk::Status::Interrupted),"Interruption reports whether replacement already happened");
    campaign::CampaignCore loaded(rules::Difficulty::Hard);Check(disk::LoadFile(loaded,slot).status==disk::Status::Ok && loaded.Now()==(replaced?9:1),"Cancellation always leaves one complete primary");
    if(!replaced)Check(Read(slot)==original,"Precommit interruption keeps exact original bytes");
    for(const auto& entry:fs::directory_iterator(f.path))Check(entry.path().filename().string().find(".thug-tmp.")==std::string::npos,"Graceful interruption cleans owned temporaries");
  }
}
void PathsAndConcurrency(){
  Folder f;campaign::CampaignCore core(rules::Difficulty::Hard);const auto slot=f.path/"slot.sav";
  Check(disk::LoadFile(core,slot).status==disk::Status::NotFound,"Fresh slot is distinguished from corrupt data");
  Check(disk::SaveFile(core,f.path/"missing"/"slot.sav").status==disk::Status::InvalidPath,"Caller must provision parent directory");
  Check(disk::SaveFile(core,fs::path{}).status==disk::Status::InvalidPath,"Empty path is rejected");
  const auto directorySlot=f.path/"directory";fs::create_directory(directorySlot);
  Check(disk::LoadFile(core,directorySlot).status==disk::Status::InvalidPath,"Directory cannot be a slot");
  disk::Result blocked{disk::Status::Ok};
  Check(disk::SaveFile(core,slot,[&](disk::Stage stage){if(stage==disk::Stage::TempCreated){campaign::CampaignCore other(rules::Difficulty::Hard);blocked=disk::LoadFile(other,slot);}return true;}).status==disk::Status::Ok,"Independent contender does not interrupt owner");
  Check(blocked.status==disk::Status::Busy,"Contending slot request returns Busy without waiting");
  const auto blockedPath=f.path/"blocked.sav";
  const auto result=disk::SaveFile(core,blockedPath,[&](disk::Stage stage){if(stage==disk::Stage::BackupSynced)fs::create_directory(blockedPath);return true;});
  Check(result.status==disk::Status::IoError && fs::is_directory(blockedPath),"Failed replacement reports IO error and leaves destination intact");
  const auto orphan=f.path/"slot.sav.thug-tmp.123.456";const auto unrelated=f.path/"slot.sav.thug-tmp.user-data";
  Write(orphan,"orphan");Write(unrelated,"keep");Check(disk::SaveFile(core,slot).status==disk::Status::Ok && !fs::exists(orphan) && fs::exists(unrelated),"Cleanup recognizes only reserved numeric temporary namespace");
}
}
int main(){try{Recovery();Interruptions();PathsAndConcurrency();std::cout<<"CheckpointFile: "<<checks<<" checks passed\n";}catch(const std::exception& e){std::cerr<<"After "<<checks<<" checks: "<<e.what()<<'\n';return 1;}}
