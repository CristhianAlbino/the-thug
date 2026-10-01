#include "CheckpointFile.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
using namespace thug;
namespace disk=campaign::checkpoint;
int main(int argc,char** argv){
  if(argc<3)return 2;
  const std::string command=argv[1];const auto slot=std::filesystem::u8path(argv[2]);
  campaign::CampaignCore core(rules::Difficulty::Hard,77);
  if(command=="load"){
    const auto result=disk::LoadFile(core,slot);
    std::cout<<static_cast<int>(result.status)<<' '<<core.Now()<<'\n';return 0;
  }
  if(core.Tick(command=="seed"?1:9,survival::Activity::Rest).status!=campaign::Status::Applied)return 3;
  const int stage=argc==4?std::stoi(argv[3]):-1;
  const auto result=disk::SaveFile(core,slot,[&](disk::Stage current){
    if(command=="crash" && static_cast<int>(current)==stage)std::_Exit(73);
    if(command=="hold" && current==disk::Stage::TempCreated){
      auto marker=slot;marker+=".ready";{std::ofstream out(marker);out<<"locked\n";if(!out)std::_Exit(74);}
      std::cin.get();
    }
    return true;
  });
  return result.status==disk::Status::Ok?0:4;
}
