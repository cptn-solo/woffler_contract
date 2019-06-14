#include <player.hpp>
#include <channel.hpp>
#include <branch.hpp>
#include <branchmeta.hpp>
#include <level.hpp>

namespace Woffler {
  namespace Player {
    Player::Player(name self, name account) :
      Entity<players, DAO, name>(self, account) {}

    DAO::DAO(players& _players, uint64_t _playerV):
      Accessor<players, wflplayer, players::const_iterator, uint64_t>::Accessor(_players, _playerV) {}

    DAO::DAO(players& _players, players::const_iterator itr):
      Accessor<players, wflplayer, players::const_iterator, uint64_t>::Accessor(_players, itr) {}

    wflplayer Player::getPlayer() {
      return getEnt<wflplayer>();
    }
    
    name Player::getChannel() {
      return getPlayer().channel;
    }

    void Player::createPlayer(name payer, name referrer) {
      auto _referrer = (referrer ? referrer : _self);

      check(
        _entKey == _self || _referrer != _entKey, //only contract account can be register by his own
        "One can not be a sales channel for himself"
      );

      //channel's account must exist at the moment of player signup unless channel isn't the contract itself
      if (_referrer != _entKey)
        checkReferrer(referrer);

      //account can't be registred twice
      checkNoPlayer();

      create(payer, [&](auto& p) {
        p.account = _entKey;
        p.channel = referrer;
      });
      
      Channel::Channel channel(_self, _referrer);
      channel.upsertChannel(_self);//contract pays RAM for the sales channels' record
    }

    void Player::addBalance(asset amount, name payer) {
      checkPlayer();
      update(payer, [&](auto& p) {
        p.activebalance += amount;
      });
    }

    void Player::subBalance(asset amount, name payer) {
      checkBalanceCovers(amount);
      update(payer, [&](auto& p) {
        p.activebalance -= amount;
      });
    }

    void Player::rmAccount() {
      checkBalanceZero();
      checkNotReferrer();
      
      Channel::Channel channel(_self, getChannel());
      channel.subChannel(_self);

      remove();
    }

    void Player::switchBranch(uint64_t idbranch) {
      auto _player = getPlayer();

      check(
        _player.status != Const::playerstate::TAKE,
        "Player can not leave the game while in TAKE state. Please wait for vested funds or return (Un-take) reward first."
      );
      if (idbranch == 0) {
        switchRootLevel(0, Const::playerstate::INIT);
        return;
      }
      //find branch of the level
      Branch::Branch branch(_self, idbranch);
      branch.checkNotClosed();

      auto _branch = branch.getBranch();
      uint64_t idrootlvl = _branch.idrootlvl;
      
      Level::Level level(_self, idrootlvl);
      auto _level = level.getLevel();

      bool startJailed = false;

      if (_branch.idparent == 0) {
        //no reason to check player current state if he just want to give up his current position
        branch.checkStartBranch();
        startJailed = level.meta.getMeta().startjailed;
      } 
      else {
        check(
          _player.idlevel == _level.idparent && 
          _player.status == Const::playerstate::GREEN, 
          "Player can move to side branch only from GREEN in split level."
        );
      }

      //check if branch is unlocked (its root level is not locked)
      level.checkUnlockedLevel();

      switchRootLevel(idrootlvl, (startJailed ? Const::playerstate::RED : Const::playerstate::SAFE));
    }

    void Player::switchRootLevel(uint64_t idlevel, Const::playerstate playerState) {
      //position player in root level of the branch
      update(_entKey, [&](auto& p) {
        p.idlevel = idlevel;
        p.triesleft = Const::retriesCount;
        p.status = playerState;
        p.tryposition = 0;
        p.currentposition = 0;
        p.resulttimestamp = 0;
      });
    }

    void Player::tryTurn() {
      checkState(Const::playerstate::SAFE);
      
      auto _player = getPlayer();
      
      /* Turn logic */
      //find player's current level 
      Level::Level level(_self, _player.idlevel);
      level.checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level
      auto _level = level.getLevel();

      Branch::Branch branch(_self, _level.idbranch);
      branch.checkNotClosed();

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _level.idmeta);    
      auto _meta = meta.getMeta();

      if (_player.triesleft >= 1) {
        //get current position and produce tryposition by generating random offset
        auto rnd = randomizer::getInstance(_entKey, _player.idlevel);
        auto tryposition = (_player.currentposition + rnd.range(Const::tryturnMaxDistance)) % Const::lvlLength;
        useTry(tryposition);    
      }

      if (_player.triesleft == 0) {
        auto status = level.cellTypeAtPosition(_player.tryposition);
        commitTurn(status);
      }
    }

    void Player::commitTurn() {
      checkState(Const::playerstate::SAFE);

      auto _player = getPlayer();

      Level::Level level(_self, _player.idlevel);
      auto status = level.cellTypeAtPosition(_player.tryposition);

      Branch::Branch branch(_self, level.getLevel().idbranch);
      branch.checkNotClosed();

      commitTurn(status);
    }

    void Player::useTry() {
      auto p = getPlayer();
      useTry(p.tryposition);
    }

    void Player::useTry(uint8_t position) {
      update(_entKey, [&](auto& p) {
        p.tryposition = position;
        p.triesleft -= 1;
      });
    }

    void Player::commitTurn(Const::playerstate status) {
      update(_entKey, [&](auto& p) {
        p.currentposition = p.tryposition;
        p.status = status;
        p.resulttimestamp = Utils::now();
        p.triesleft = Const::retriesCount;
      });
    }

    void Player::commitTake(asset amount, uint32_t timestamp) {
      update(_entKey, [&](auto& p) {
        p.status = Const::playerstate::TAKE;
        p.resulttimestamp = timestamp;
        p.vestingbalance += amount;
        //triesleft must remain as before action to prevent "free" bets upon un-take
      });
    }

    void Player::cancelTake() {
      checkState(Const::playerstate::TAKE);

      auto _player = getPlayer();

      Level::Level level(_self, _player.idlevel);
      Branch::Branch branch(_self, level.getLevel().idbranch);
      branch.checkNotClosed();//must be safe to claim takes on closed branches

      branch.addPot(_entKey, _player.vestingbalance);
      level.addPot(_entKey, _player.vestingbalance);

      update(_entKey, [&](auto& p) {
        p.status = Const::playerstate::GREEN;
        p.resulttimestamp = Utils::now();
        p.vestingbalance = asset{0, Const::acceptedSymbol};
        //triesleft must remain as before take to prevent "free bets"
      });      
    }

    void Player::claimSafe() {
      auto _player = getPlayer();
      
      Level::Level level(_self, _player.idlevel);
      Branch::Branch branch(_self, level.getLevel().idbranch);      
      branch.checkNotClosed();
      
      check(
        _player.status == Const::playerstate::GREEN ||
        _player.status == Const::playerstate::NEXT ||
        _player.status == Const::playerstate::SPLIT,
        "Only player in GREEN/NEXT/SPLIT state can apply for repositon to 0 cell (safe)."
      );

      resetPositionAtLevel(_player.idlevel);
    }

    void Player::claimRed() {
      checkState(Const::playerstate::RED);

      auto _player = getPlayer();
      
      Level::Level level(_self, _player.idlevel);
      auto _level = level.getLevel();
      auto _meta = level.meta.getMeta();
      check(
        _level.idparent != 0 || !_meta.startjailed,
        "You can only call `unjail` or `switchbrnch` from your current state"
      );
      uint64_t idlevel = (_level.idparent != 0 ? _level.idparent : _level.id);
      resetPositionAtLevel(idlevel);
    }

    void Player::claimTake() {      
      checkState(Const::playerstate::TAKE);

      auto _player = getPlayer();
      
      Level::Level level(_self, _player.idlevel);
      Branch::Branch branch(_self, level.getLevel().idbranch);
      
      if (branch.getBranch().closed == 0) {//must be safe to claim takes on closed branches

        if (_player.resulttimestamp > Utils::now()) {
          auto expiredAfter = _player.resulttimestamp - Utils::now();
          check(
            expiredAfter <= 0,
            string("TAKE state did not expired yet. Seconds left until expiration: ") + std::to_string(expiredAfter)
          );        
        }    
        resetPositionAtLevel(_player.idlevel);
      } else {
        switchRootLevel(0, Const::playerstate::INIT);
      }
      //Move player's vested balance to active balance
      update(_entKey, [&](auto& p) {
        p.activebalance += p.vestingbalance;
        p.vestingbalance = asset{0, Const::acceptedSymbol};
      });
    }

    void Player::resetPositionAtLevel(uint64_t idlevel) {
      update(_entKey, [&](auto& p) {
        p.idlevel = idlevel;
        p.tryposition = 0;
        p.currentposition = 0;
        p.status = Const::playerstate::SAFE;
        p.resulttimestamp = 0;
        p.triesleft = Const::retriesCount;
      });
    }

    void Player::resetRetriesCount() {
      update(_entKey, [&](auto& p) {
        p.triesleft = Const::retriesCount;
      });
    }

    bool Player::isPlayer() {
      return isEnt();
    }

    void Player::checkReferrer(name referrer) {
      check(
        isEnt(referrer),
        string("Account ") + referrer.to_string() + string(" is not registred in game conract.")
      );
    }

    void Player::checkNotReferrer() {
      auto player = getPlayer();
      auto idxchannel = getIndex<"bychannel"_n>();
      auto itrchannel = idxchannel.find(_entKey.value);
      check(
        itrchannel == idxchannel.end(),
        string("Can't remove account ") + _entKey.to_string() + string(" as it is registred as a referrer for other accounts.")
      );
      return ;
    }

    void Player::checkPlayer() {
      check(
        isEnt(),
        string("Account ") + _entKey.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
      );
    }

    void Player::checkNoPlayer() {
      check(
        !isEnt(),
        string("Account ") + _entKey.to_string() + string(" is already registred.")
      );
    }

    void Player::checkActivePlayer() {
      auto p = getPlayer();
      check(
        p.idlevel != 0,
        "First select branch to play on with action switchbrnch."
      );
    }

    void Player::checkState(Const::playerstate state) {
      auto p = getPlayer();
      checkActivePlayer();
      check(
        p.status == state,
        string("Player current level resutl must be '") + std::to_string(state) + string("'.")
      );
    }

    void Player::checkBalanceCovers(asset amount) {
      auto p = getPlayer();
      check(
        p.activebalance >= amount,
        string("Not enough active balance in your account. Current active balance: ") + p.activebalance.to_string().c_str()
      );
    }

    void Player::checkBalanceZero() {
      auto p = getPlayer();
      check(//warning! works only for records, emplaced in contract's host scope
        p.activebalance == asset{0, Const::acceptedSymbol},
        string("Please withdraw funds first. Current active balance: ") + p.activebalance.to_string().c_str()
      );
    }

    void Player::checkLevelUnlockTrialAllowed() {
      auto p = getPlayer();
      check(
        p.status == Const::playerstate::NEXT ||
        p.status == Const::playerstate::SPLIT,
        "Player can unlock level only from NEXT/SPLIT states"
      );
      check(
        p.triesleft >= 1,
        "Retries count to unlock level is restricted. Buy next set of retries with 'buytries' action"
      );
    }

    //DEBUG only, payer == contract
    void Player::reposition(uint64_t idlevel, uint8_t position) {
      update(_self, [&](auto& p) {
        p.idlevel = idlevel;
        p.tryposition = position;
        p.triesleft = Const::retriesCount;
      });
    }
    
    void Player::rmPlayer() {
      remove();
    }
  }
} // namespace Woffler
