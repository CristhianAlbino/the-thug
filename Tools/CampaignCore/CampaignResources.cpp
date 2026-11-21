#include "CampaignState.h"
#include "CompanionIO.h"
#include "CombatIO.h"
#include "SpecialIO.h"
#include "InteractionIO.h"
#include "RescueIO.h"
#include "StrategyIO.h"
#include <algorithm>

namespace thug::campaign {
namespace {
template<class T> const T* Find(const std::vector<T>& list,const std::string& id) {
  for(const auto& value:list) if(value.id==id) return &value;
  return nullptr;
}
}
// Validate total stock across every command family, including supplies left on
// the floor. Reservations keep stock in its owner's bag until real completion.
bool CampaignCore::ResourceCoherent(const State& s) const {
  std::map<std::string,std::map<std::string,long long>> bags;
  for(const auto& [id,n]:s.profile.loadout) bags["joe"][id]=n;
  const auto party=partydata::For(s.routes.DifficultyLevel());
  int index=0;
  for(const auto& actor:{"harley","bartholomeu","jack"}) {
    for(const auto& [id,n]:party.loadouts[index]) bags[actor][id]=n;
    ++index;
  }
  const auto change=[&](const std::string& actor,const std::string& item,int n) {
    if(!item.empty()) bags[actor][item]+=n;
  };
  const auto use=[&](const std::string& actor,const std::string& id) {
    const auto* u=Find(definition_->uses,id); if(!u) return false;
    if(u->consume) change(actor,u->item,-u->quantity);
    return true;
  };
  std::map<std::uint64_t,SpecialCommand> special;
  std::map<std::uint64_t,RescueCommand> rescue;
  std::map<std::uint64_t,StrategyCommand> strategy;
  for(const auto& [id,signature]:s.receipts) {
    if(signature.rfind("C ",0)==0) {
      CompanionCommand c; if(!detail::ParseCompanion(signature,c)) return false;
      if(c.kind==CompanionOp::Transfer) { change(c.actor,c.item,-c.quantity); change(c.target,c.item,c.quantity); }
      if(c.kind==CompanionOp::Treat || c.kind==CompanionOp::Stabilize) change(c.actor,c.item,-1);
      if(c.kind==CompanionOp::Complete) {
        if(c.argument==static_cast<int>(companion::Goal::SearchResources)) {
          const auto* p=Find(definition_->pickups,c.target); if(!p) return false; change(c.actor,p->item,p->quantity);
        }
        if(c.argument==static_cast<int>(companion::Goal::HelpJoe)) change(c.actor,"treatment",-1);
      }
      if(c.kind==CompanionOp::Operate) {
        if(c.argument==static_cast<int>(ActionKind::Use) && !use(c.actor,c.target)) return false;
        if(c.argument==static_cast<int>(ActionKind::UnlockDoor)) {
          const auto* u=Find(definition_->unlocks,c.target); if(!u) return false; if(u->consume) change(c.actor,u->item,-u->quantity);
        }
      }
    } else if(signature.rfind("W ",0)==0) {
      CombatCommand c; std::uint64_t order,created; if(!detail::ParseCombat(signature,c,order,created)) return false;
      if(c.kind==CombatOp::CompleteReload) change(c.actor,"magazine",-1);
      if(c.kind==CombatOp::ReleaseThrow) change(c.actor,c.target,-1);
    } else if(signature.rfind("S ",0)==0) {
      SpecialCommand c; c.eventId=id; std::uint64_t order; if(!detail::ParseSpecial(signature,c,order)) return false;
      special.emplace(order,c);
    } else if(signature.rfind("R ",0)==0) {
      RescueCommand c; std::uint64_t order,a,p; bool seen; if(!detail::ParseRescue(signature,c,order,seen,a,p)) return false;
      rescue.emplace(order,c);
    } else if(signature.rfind("Q ",0)==0) {
      StrategyCommand c; std::uint64_t order,a,p; bool accepted; if(!detail::ParseStrategy(signature,c,order,accepted,a,p)) return false;
      if(accepted) strategy.emplace(order,c);
    } else if(signature.rfind("I ",0)==0) {
      InteractionCommand c; std::uint64_t order; bool accepted; if(!detail::ParseInteraction(signature,c,order,accepted)) return false;
      if(!accepted) continue;
      if(c.kind==InteractionOp::PlaceWedge) change("joe",c.item,-1);
      if(c.kind==InteractionOp::RecoverWedge) change("joe",c.item,1);
      if(c.kind==InteractionOp::Gesture && c.argument==1) change(c.actor,c.item,-1);
      if(c.kind==InteractionOp::Gesture && c.argument==2) { change(c.actor,c.item,-1); change("joe",c.item,1); }
    } else if(signature.rfind("H ",0)!=0) {
      Action c; if(!detail::ParseSignature(signature,c)) return false;
      if(c.kind==ActionKind::Collect || c.kind==ActionKind::Drop) {
        const auto* p=Find(definition_->pickups,c.target); if(!p) return false;
        change("joe",p->item,c.kind==ActionKind::Collect ? p->quantity : -p->quantity);
      }
      if(c.kind==ActionKind::Treat || c.kind==ActionKind::Stabilize) change("joe",c.target,-1);
      if(c.kind==ActionKind::Use && !use("joe",c.target)) return false;
      if(c.kind==ActionKind::SubmitKeypad && c.target==director::Content().garageCode && !use("joe","safe_code_accepted")) return false;
      if(c.kind==ActionKind::UnlockDoor) { const auto* u=Find(definition_->unlocks,c.target); if(!u) return false; if(u->consume) change("joe",u->item,-u->quantity); }
      if(c.kind==ActionKind::ShareAmmo) { change("joe","magazine",-1); change(c.target,"magazine",1); }
    }
  }
  std::set<std::string> recruited;
  for(const auto& [order,c]:special) {
    (void)order;
    if(c.kind==SpecialOp::Recruit) recruited.insert(c.target);
    if(c.kind==SpecialOp::StartConfrontation) {
      change("joe","magazine",-1); change("joe","smoke",-1);
      for(const auto& actor:recruited) change(actor,"magazine",-1);
    }
    if(c.kind==SpecialOp::AcquireSupply) {
      if(!definition_->finale) return false;
      const auto* p=Find(definition_->finale->supplies,c.target); if(!p) return false; change("joe",p->item,p->quantity);
    }
    if(c.kind==SpecialOp::CompletePlant) change("joe","structural_charge",-1);
    if(c.kind==SpecialOp::DetonateCollapse) change("joe","structural_detonator",-1);
  }
  for(const auto& [order,c]:rescue) {
    (void)order;
    if(c.kind==RescueOp::CompleteRelease) {
      const auto begin=rescue.find(c.serial); if(begin==rescue.end()) return false;
      if(!begin->second.item.empty()) change(c.actor,begin->second.item,-1);
    }
  }
  for(const auto& [order,c]:strategy) {
    (void)order;
    if(c.kind==StrategyOp::DropItem) change(c.actor,c.item,-c.argument);
    if(c.kind==StrategyOp::TakeDrop) {
      const auto drop=s.strategy.drops.find(c.target); if(drop==s.strategy.drops.end()) return false;
      change(c.actor,drop->second.item,c.argument);
    }
    if(c.kind==StrategyOp::CompleteJointUse) {
      const auto begin=strategy.find(c.serial); if(begin==strategy.end() || !use(c.target,begin->second.reference)) return false;
    }
  }
  for(const auto& actor:{"joe","harley","bartholomeu","jack"}) {
    const auto& bag=std::string(actor)=="joe" ? s.inventory : s.bodies.at(actor).inventory;
    for(const auto& item:data::Items()) if(bags[actor][item.id]!=bag.Quantity(item.id)) return false;
    for(const auto& [id,n]:bags[actor]) if(n<0 || n>100 || (n && bag.Quantity(id)!=n)) return false;
  }
  return true;
}
} // namespace thug::campaign
