#include <player.hpp>
#include <channel.hpp>
#include <branch.hpp>
#include <branchmeta.hpp>
#include <level.hpp>

namespace Woffler {
  namespace Player {
    void Player::createPlayer(const name& payer, const name& referrer) {
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

      _player = create(payer, [&](auto& p) {
        p.account = _entKey;
        p.channel = referrer;
      });
      
      Channel::Channel channel(_self, _referrer);
      channel.upsertChannel(_self);//contract pays RAM for the sales channels' record
    }

    void Player::addBalance(const asset& amount, const name& payer) {
      checkPlayer();
      _player = update(payer, [&](auto& p) {
        p.activebalance += amount;
      });
    }

    void Player::subBalance(const asset& amount, const name& payer) {
      checkBalanceCovers(amount);
      _player = update(payer, [&](auto& p) {
        p.activebalance -= amount;
      });
    }

    void Player::claimVesting() {
      _player = update(_player.account, [&](auto& p) {
        p.activebalance += p.vestingbalance;
        p.vestingbalance = asset{0, Const::acceptedSymbol};
      });
    }

    void Player::clearVesting() {
      _player = update(_player.account, [&](auto& p) {
        p.status = Const::playerstate::GREEN;
        p.resulttimestamp = Utils::now();
        p.vestingbalance = asset{0, Const::acceptedSymbol};
        //triesleft must remain as before take to prevent "free bets"
      });      
    }

    void Player::rmAccount() {
      checkBalanceZero();
      checkNotReferrer();
      
      Channel::Channel channel(_self, getChannel());
      channel.subChannel(_self);

      remove();
    }

    void Player::switchBranch(const uint64_t& idbranch) {
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

    void Player::switchRootLevel(const uint64_t& idlevel, const Const::playerstate& playerState) {
      //position player in root level of the branch
      _player = update(_entKey, [&](auto& p) {
        p.idlevel = idlevel;
        p.triesleft = Const::retriesCount;
        p.status = playerState;
        p.tryposition = 0;
        p.currentposition = 0;
        p.resulttimestamp = 0;
      });
    }

    void Player::useTry() {
      useTry(_player.tryposition);
    }

    void Player::useTry(const uint8_t& position) {
      _player = update(_entKey, [&](auto& p) {
        p.tryposition = position;
        p.triesleft -= 1;
      });
    }

    void Player::commitTurn(const Const::playerstate& status) {
      _player = update(_entKey, [&](auto& p) {
        p.currentposition = p.tryposition;
        p.status = status;
        p.resulttimestamp = Utils::now();
        p.triesleft = Const::retriesCount;
      });
    }

    void Player::commitTake(const asset& amount, const uint32_t& timestamp) {
      _player = update(_entKey, [&](auto& p) {
        p.status = Const::playerstate::TAKE;
        p.resulttimestamp = timestamp;
        p.vestingbalance += amount;
        //triesleft must remain as before action to prevent "free" bets upon un-take
      });
    }    

    void Player::resetPositionAtLevel(const uint64_t& idlevel) {
      _player = update(_entKey, [&](auto& p) {
        p.idlevel = idlevel;
        p.tryposition = 0;
        p.currentposition = 0;
        p.status = Const::playerstate::SAFE;
        p.resulttimestamp = 0;
        p.triesleft = Const::retriesCount;
      });
    }

    void Player::resetRetriesCount() {
      _player = update(_entKey, [&](auto& p) {
        p.triesleft = Const::retriesCount;
      });
    }

    bool Player::isPlayer() {
      return isEnt();
    }

    void Player::checkReferrer(const name& referrer) {
      check(
        isEnt(referrer),
        string("Account ") + referrer.to_string() + string(" is not registred in game conract.")
      );
    }

    void Player::checkNotReferrer() {
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
      check(
        _player.idlevel != 0,
        "First select branch to play on with action switchbrnch."
      );
    }

    void Player::checkState(const Const::playerstate& state) {
      checkActivePlayer();
      check(
        _player.status == state,
        string("Player current level resutl must be '") + std::to_string(state) + string("'.")
      );
    }

    void Player::checkBalanceCovers(const asset& amount) {
      check(
        _player.activebalance >= amount,
        string("Not enough active balance in your account. Current active balance: ") + _player.activebalance.to_string().c_str()
      );
    }

    void Player::checkBalanceZero() {
      check(//warning! works only for records, emplaced in contract's host scope
        _player.activebalance == asset{0, Const::acceptedSymbol},
        string("Please withdraw funds first. Current active balance: ") + _player.activebalance.to_string().c_str()
      );
    }

    void Player::checkLevelUnlockTrialAllowed() {
      check(
        _player.status == Const::playerstate::NEXT ||
        _player.status == Const::playerstate::SPLIT,
        "Player can unlock level only from NEXT/SPLIT states"
      );
      check(
        _player.triesleft >= 1,
        "Retries count to unlock level is restricted. Buy next set of retries with 'buytries' action"
      );
    }

    //DEBUG only, payer == contract
    void Player::reposition(const uint64_t& idlevel, const uint8_t& position) {
      _player = update(_self, [&](auto& p) {
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
