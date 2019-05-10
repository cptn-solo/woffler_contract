#include <woffler.hpp>
#include "wflPlayer.cpp"
#include "wflBranch.cpp"
#include "wflLevel.cpp"

//*** Contract scope methods ***//
namespace woffler {

  #pragma region ** wflPlayer**

  void woffler::claimtake(name player) {

  }
  
  #pragma endregion

  #pragma region ** wflBranch**

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

  #pragma endregion   
  
  #pragma region wflLevel

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


  
  #pragma endregion
}