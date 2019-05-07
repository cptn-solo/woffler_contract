#include <woffler.hpp>
#include "wflPlayer.cpp"
#include "wflBranch.cpp"
#include "wflLevel.cpp"

//*** Contract scope methods ***//
namespace woffler {

  #pragma region ** wflPlayer**

  void woffler::tryturn(name player) {
    require_auth(player);
    
    auto self = get_self();

    Player _player(self, player);
    _player.checkState(Const::playerstate::SAFE);
    
    /* Turn logic */
    //find player's current level 
    Level _level(self, (*_player.player)->idlvl);
    _level.checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level

    //getting branch meta to decide on level presets
    brnchmetas _metas(self, self.value);    
    auto _meta = _metas.find(_level.idmeta);

    if ((*_player.player)->triesleft >= 1) {
      //get current position and produce tryposition by generating random offset
      auto rnd = randomizer::getInstance(player, (*_player.player)->idlvl);
      auto tryposition = ((*_player.player)->currentposition + rnd.range(Const::tryturnMaxDistance)) % _meta->lvllength;
      _player.useTry(tryposition);    
    }

    if ((*_player.player)->triesleft == 0) {
      Const::playerstate levelresult = _level.cellTypeAtPosition((*_player.player)->tryposition);
      _player.commitTurn(levelresult);
    }
  }

  void woffler::committurn(name player) {
    require_auth(player);
    
    auto self = get_self();

    Player _player(self, player);
    _player.checkState(Const::playerstate::SAFE);

    Level _level(self, (*_player.player)->idlvl);
    _level.checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level

    auto levelresult = _level.cellTypeAtPosition((*_player.player)->tryposition);
    _player.commitTurn(levelresult);
  }

  void woffler::claimred(name player) {
    require_auth(player);

    auto self = get_self();

    Player _player(self, player);
    _player.checkState(Const::playerstate::RED);

    Level _level(self, (*_player.player)->idlvl);
    _level.checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level

    /* Claim logic */  
    uint64_t idlevel = (_level.idparent > 0 ? _level.idparent : _level.idlevel);
    _player.resetPositionAtLevel(idlevel);
  }

  void woffler::claimgreen(name player) {
    
    require_auth(player);          

    Player _player(get_self(), player);
    _player.checkState(Const::playerstate::GREEN);
    
    /* Claim logic */  
    _player.resetPositionAtLevel((*_player.player)->idlvl);
  }

  void woffler::claimtake(name player) {

  }
  
  #pragma endregion

  #pragma region ** wflBranch**

  void woffler::branch(name owner, uint64_t idmeta, asset pot) {
    require_auth(owner);
    print("Creating branch with idmeta: ", std::to_string(idmeta));
    
    //create root branch with root level after meta is created/selected from existing
    auto self = get_self();
    
    brnchmetas _metas(self, self.value);    
    auto _meta = _metas.find(idmeta);
    check(
      _meta != _metas.end(),
      "No branch metadata found for id"
    );

    auto minPot = (((_meta->stkmin * 100) / _meta->stkrate) * 100) / _meta->spltrate;
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
    Player _player(self, owner);
    _player.subBalance(pot, owner);

    Branch _branch(self, 0);
    _branch.createBranch(owner, idmeta);
    
    //register players's and house stake
    auto playerStake = (pot * (100 - Const::houseShare)) / 100;
    registerStake(owner, _branch.idbranch, playerStake);

    auto houseStake = (pot * Const::houseShare) / 100;
    registerStake(self, _branch.idbranch, houseStake);

    print(" Root branch created, id: ", std::to_string(_branch.idbranch), ", pot: ", asset{pot});
  }

  void woffler::rootlvl(name owner, uint64_t idbranch) {
    require_auth(owner);
    print("Creating root level for branch with id: ", std::to_string(idbranch));
    
    auto self = get_self();
    
    Branch _branch(self, idbranch);
    _branch.checkEmptyBranch();

    //find stake to use as pot value for root level
    stakes _stakes(self, self.value);

    auto ownedBranchId = Utils::combineIds(owner.value, idbranch);    
    auto stkidx = _stakes.get_index<name("byownedbrnch")>();
    auto stake = stkidx.find(ownedBranchId);          

    check(
      stake != stkidx.end(),
      "Only stakeholder of a branch can create root level for it"
    );
    
    //add pot value from owner's active balance to the root level's pot
    uint64_t idrootlvl = addLevel(owner, idbranch, _branch.idmeta);
    _branch.setRootLevel(owner, idrootlvl);
  }

  void woffler::addquest(name owner, uint64_t idbranch, 
    uint64_t idquest
  ) {
    require_auth(owner);
    check(false, "Not implemented");

  }

  void woffler::rmquest(name owner, uint64_t idbranch, 
    uint64_t idquest
  ) {
    require_auth(owner);
    check(false, "Not implemented");

  }

  //DEBUG: update idrootlvl
  void woffler::setrootlvl(name owner, uint64_t idbranch, uint64_t idrootlvl) {
    require_auth(owner);
    auto self = get_self();
    check(
      owner == self,
      string("Debug mode available only to contract owner: ") + self.to_string()
    );

    Branch _branch(self, idbranch);
    _branch.checkBranch();

    _branch.setRootLevel(owner, idrootlvl);
  }
  #pragma endregion   
  
  #pragma region wflLevel
  
  uint64_t woffler::addLevel(name owner, uint64_t idbranch, uint64_t idmeta) {
    auto self = get_self();
    
    //find stake to use as pot value for root level
    stakes _stakes(self, self.value);

    //calculating branch stake total (all stakeholders)
    auto stkidx = _stakes.get_index<name("bybranch")>();
    auto stkitr = stkidx.lower_bound(idbranch);
    auto branchStake = asset{0, Const::acceptedSymbol};
    while(stkitr != stkidx.end()) {
      branchStake+=stkitr->stake;
      stkitr++;
    }

    //getting branch meta to decide on level presets
    brnchmetas _metas(self, self.value);    
    auto _meta = _metas.find(idmeta);
    check(
      _meta != _metas.end(),
      "No branch metadata found for branch"
    );

    //emplacing new (root) level
    Level _level(self, 0);
    _level.createLevel(owner, branchStake, idbranch, idmeta, _meta->lvlreds, _meta->lvllength);

    print("Root level created with id: ", std::to_string(_level.idlevel) , ", pot balance: ", asset{branchStake}, " for branch: ", std::to_string(idbranch));    
    
    return _level.idlevel;
  }

  void woffler::unlocklvl(name owner, uint64_t idlevel) {
    require_auth(owner);
    auto self = get_self();
    
    //find level to unlock
    Level _level(self, idlevel);
    _level.checkLockedLevel();

    //find player
    Player _player(self, owner);
    _player.checkPlayer();

    /* Restrictions check */
    if (_level.root) {//root level can be unlocked only by stakeholder, unlimited retries count
      //find stake to use as pot value for root level
      stakes _stakes(self, self.value);

      auto ownedBranchId = Utils::combineIds(owner.value, _level.idbranch);    
      auto stkidx = _stakes.get_index<name("byownedbrnch")>();
      const auto& stake = stkidx.find(ownedBranchId);          

      check(
        stake != stkidx.end(),
        "Only stakeholder of a branch can unlock root level for it"
      );
    }
    else {//"next" level can be unlocked only from GREEN position, retries count limited
      _player.checkLevelUnlockTrialAllowed(_level.idparent);
      _player.useTry();
    }

    /* Generate cells */  

    //getting branch meta to decide on level presets
    brnchmetas _metas(self, self.value);    
    auto _meta = _metas.find(_level.idmeta);
    
    _level.unlockTrial(owner, _meta->lvlgreens, _meta->lvllength);

    if (!_level.root && !_level.locked) {
      //process NEXT workflow: position player to the unlocked level
      _player.resetPositionAtLevel(_level.idlevel);
    }
  }

  void woffler::nextlvl(name player) {
    require_auth(player);
  }

  void woffler::takelvl(name player) {
    require_auth(player);
    //dont forget to set retries count = 0 to force a player to call `splitbet` before split branch unlock trial
  }

  void woffler::splitlvl(name player) {
    require_auth(player);  
  }

  void woffler::splitbet(name player) {
    require_auth(player);
    //players in TAKE state 1st cut vested balance, then - active
    //reset retries count if balance cut was successfull
  }

  //DEBUG: testing cells generation for a given level and meta
  void woffler::regencells(name owner, uint64_t idlevel) {
    require_auth(owner);
    auto self = get_self();
    check(
      owner == self,
      string("Debug mode available only to contract owner: ") + self.to_string()
    );

    Level _level(self, self.value);
    _level.checkLevel();

    //getting branch meta to decide on level presets
    brnchmetas _metas(self, self.value);    
    auto _meta = _metas.find(_level.idmeta);
    check(
      _meta != _metas.end(),
      "No branch metadata found."
    );

    _level.generateRedCells(owner, _meta->lvlreds, _meta->lvllength);
    _level.unlockTrial(owner, _meta->lvlgreens, _meta->lvllength);
  }

  //DEBUG: testing cell randomizer
  void woffler::gencells(name account, uint8_t size, uint8_t maxval) {
    require_auth(account);
    auto self = get_self();
    check(
      account == self,
      string("Debug mode available only to contract owner: ") + self.to_string()
    );

    Level::debugGenerateCells(account, 1, size, maxval);
  }

  //DEBUG: testing level delete
  void woffler::rmlevel(name owner, uint64_t idlevel) {
    require_auth(owner);
    auto self = get_self();
    check(
      owner == self,
      string("Debug mode available only to contract owner: ") + self.to_string()
    );

    Level _level(self, self.value);
    _level.rmLevel();
  }

  #pragma endregion

  #pragma region wflChannel
  
  //*** Sales channel scope methods (wflChannel) ***//

  void woffler::chnmergebal(name owner) {
    auto self = get_self();
    require_auth(owner);
    
    channels _channels(self, self.value);
    auto achannel = _channels.find(owner.value);

    check(
      achannel != _channels.end(),
      "No channel found"
    );
    
    auto amount = achannel->balance;

    _channels.modify(achannel, owner, [&](auto& c) {
      c.balance = asset{0, Const::acceptedSymbol};     
    });

    Player _player(self, owner);
    _player.addBalance(amount, owner);
    
    print("Channel balance merged: ", asset{amount});
  }
  
  #pragma endregion

  #pragma region wflQuest
  
  void woffler::qstsetmeta(name owner, uint64_t idquest, 
    std::vector<uint64_t> hashes, 
    asset minprice, 
    asset maxprice, 
    string apiurl
  ) {
    require_auth(owner);
    check(false, "Not implemented");
      
  }

  void woffler::qstsetbal(name owner, uint64_t idquest, 
    asset amount
  ) {
    require_auth(owner);
    check(false, "Not implemented");

  }
  
  #pragma endregion

  #pragma region wflStake
  
  void woffler::stkaddval(name owner, uint64_t idbranch, asset amount) {
    require_auth(owner);
    
    auto self = get_self();
    Branch _branch(self, idbranch);
    _branch.checkBranch();

    //cut owner's active balance for pot value (will fail if not enough funds)
    Player _player(self, owner);
    _player.subBalance(amount, owner);
    
    if (_branch.generation > 1) {
      //non-root branches don't directly share profit with contract's account (house)
      registerStake(owner, idbranch, amount);
    } 
    else {
      //register players's and house stake
      auto playerStake = (amount * (100 - Const::houseShare)) / 100;
      registerStake(owner, idbranch, playerStake);

      auto houseStake = (amount * Const::houseShare) / 100;
      registerStake(self, idbranch, houseStake);
    }

    //if root level is created already - append staked value to the root level's pot
    if(_branch.idrootlvl > 0) {
      Level _level(self, _branch.idrootlvl);
      _level.checkLevel();
      _level.addPot(owner, amount);
      print("Value added to the pot: ", asset{amount}, ", current pot value: ", asset{_level.potbalance});  
    }
  }

  void woffler::stktakervn(name owner, uint64_t idbranch) {
    require_auth(owner);
    check(false, "Not implemented");
  }

  void woffler::registerStake(name owner, uint64_t idbranch, 
    asset amount
  ) {
    auto self = get_self();
    stakes _stakes(self, self.value);

    //find stake and add amount, or emplace if not found
    auto ownedBranchId = Utils::combineIds(owner.value, idbranch);    
    auto stkidx = _stakes.get_index<name("byownedbrnch")>();
    const auto& stake = stkidx.find(ownedBranchId);          

    auto currentStake = amount;        

    if (stake == stkidx.end()) {
      auto idstake = Utils::nextPrimariKey(_stakes.available_primary_key());
      _stakes.emplace(owner, [&](auto& s) {
        s.id = idstake;
        s.idbranch = idbranch;
        s.owner = owner;
        s.stake = amount;
        s.revenue = asset{0, Const::acceptedSymbol};//only for emplace. modify should not change revenue!
      });
    } 
    else {
      _stakes.modify(*stake, owner, [&](auto& s) {
        s.stake += amount;     
      });    
      currentStake = stake->stake;
    }

    print(" Branch stake registred, current stake owned: ", asset{currentStake});
  }
  
  #pragma endregion
}