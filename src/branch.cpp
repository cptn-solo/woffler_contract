#include <branch.hpp>
#include <branchmeta.hpp>
#include <player.hpp>
#include <stake.hpp>

namespace Woffler {
  namespace Branch {    
    Branch::Branch(name self, uint64_t idbranch) : 
      Entity<branches, DAO, uint64_t>(self, idbranch) {}

    DAO::DAO(branches& _branches, uint64_t idbranch): 
        Accessor<branches, wflbranch, branches::const_iterator, uint64_t>::Accessor(_branches, idbranch) {}

    uint64_t Branch::getRootLevel() {
      auto b = getEnt<wflbranch>();
      return b.idrootlvl;
    }

    //create root branch with root level after meta is created/selected from existing
    void Branch::createBranch(name owner, uint64_t idmeta, asset pot) {
      BranchMeta::BranchMeta meta(_self, idmeta);    
      BranchMeta::wflbrnchmeta _meta = meta.getMeta();

      auto minPot = (((_meta.stkmin * 100) / _meta.stkrate) * 100) / _meta.spltrate;
      check(
        /*
        pot - value to be placed to the root level upon creation; must be covered by creator's active balance;
        spltrate - % of level's current pot moved to new branch upon split action from winner;
        stkrate - % of level's current pot need to be staked by each pretender to be a stakeholder of the branch upon split;
        stkmin - minimum value accepted as "stake" for the branch;

        pot * spltrate% * stkrate% > stkmin
        (((pot * spltrate)/100) * stkrate)/100 > stkmin
        (((pot * spltrate)/100) * stkrate) > stkmin * 100
        ((pot * spltrate)/100) > (stkmin * 100) / stkrate
        pot > (((stkmin * 100) / stkrate) * 100) / spltrate

        for stkmin = 10, stkrate = 3, spltrate = 50 we'll get minimum pot value as
        (((10*100)/3)*100)/50 = 666

        for stkmin = 1, stkrate = 10, spltrate = 50 we'll get minimum pot value as
        (((1*100)/10)*100)/50 = 20
        
        */
        minPot <= pot,    
        string("Branch minimum pot is ")+minPot.to_string().c_str()
      );
      
      //cut owner's active balance for pot value (will fail if not enough funds)
      Player::Player player(_self, owner);
      player.subBalance(pot, owner);      
      
      //create branch record
      _entKey = nextPK();      
      create(owner, [&](auto& b) {
        b.id = _entKey;
        b.idmeta = idmeta;
      });

      //register players's and house stake
      auto houseStake = (pot * Const::houseShare) / 100;
      auto playerStake = (pot - houseStake);

      Stake::Stake stake(_self, 0);
      stake.registerStake(owner, _entKey, playerStake);
      stake.registerStake(_self, _entKey, houseStake);
    }

    void Branch::createRootLevel(name owner) {
      checkEmptyBranch();

      //find stake to use as pot value for root level
      asset branchStake = asset{0, Const::acceptedSymbol};
      asset ownerStake = asset{0, Const::acceptedSymbol};

      Stake::Stake stake(_self, 0);
      stake.branchStake(owner, _entKey, branchStake, ownerStake);

      check(
        ownerStake > asset{0, Const::acceptedSymbol},
        "Only root branch stakeholder allowed to create a root level for the branch"
      );
      
      //add pot value from owner's active balance to the root level's pot
      uint64_t idrootlvl = addLevel(owner, branchStake);      
      setRootLevel(owner, idrootlvl);
    }
    
    uint64_t Branch::addLevel(name owner, asset pot) {      
      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, getEnt<wflbranch>().idmeta);    
      BranchMeta::wflbrnchmeta _meta = meta.getMeta();

      //emplacing new (root) level
      Level::Level level(_self, 0);
      uint64_t idlevel = level.createLevel(owner, pot, _entKey, _meta);

      return idlevel;
    }

    void Branch::setRootLevel(name payer, uint64_t idrootlvl) {
      checkBranch();
      update(payer, [&](auto& b) {
        b.idrootlvl = idrootlvl;
      });    
    }

    void Branch::checkBranch() {
      check(
        isEnt(),
        "No branch found for id"
      );
    }

    void Branch::checkStartBranch() {
      auto b = getEnt<wflbranch>();
      check(
        b.generation == 1,
        "Player can start only from root branch"
      );
      check(
        b.idrootlvl != 0,
        "Branch has no root level yet."
      );
    }  

    void Branch::checkEmptyBranch() {
      auto b = getEnt<wflbranch>();
      check(
        b.idrootlvl == 0,
        "Root level already exists"
      );
    }  

    void Branch::checkBranchMetaNotUsed(uint64_t idmeta) {
      check(
        !isIndexedByMeta(idmeta),
        "Branch metadata is already used in branches."
      );
    }
    
    bool Branch::isIndexedByMeta(uint64_t idmeta) {
        auto idxbymeta = getIndex<"bymeta"_n>();
        auto itrbymeta = idxbymeta.find(idmeta);  
        return itrbymeta != idxbymeta.end();
    }    
  }
}