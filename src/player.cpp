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
      checkSwitchBranchAllowed();

      //find branch of the level
      Branch::Branch branch(_self, idbranch);
      branch.checkStartBranch();

      uint64_t idrootlvl = branch.getRootLevel();

      //check if branch is unlocked (its root level is not locked)
      Level::Level level(_self, idrootlvl);
      level.checkUnlockedLevel();

      switchRootLevel(idrootlvl);
    }

    void Player::switchRootLevel(uint64_t idlvl) {
      //position player in root level of the branch
      update(_entKey, [&](auto& p) {
        p.idlvl = idlvl;
        p.triesleft = Const::retriesCount;
        p.levelresult = Const::playerstate::SAFE;
        p.tryposition = 0;
        p.currentposition = 0;
      });
    }

    void Player::tryTurn() {
      checkState(Const::playerstate::SAFE);
      
      auto _player = getPlayer();
      
      /* Turn logic */
      //find player's current level 
      Level::Level level(_self, _player.idlvl);
      level.checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level
      auto _level = level.getLevel();

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _level.idmeta);    
      auto _meta = meta.getMeta();

      if (_player.triesleft >= 1) {
        //get current position and produce tryposition by generating random offset
        auto rnd = randomizer::getInstance(_entKey, _player.idlvl);
        auto tryposition = (_player.currentposition + rnd.range(Const::tryturnMaxDistance)) % Const::lvlLength;
        useTry(tryposition);    
      }

      if (_player.triesleft == 0) {
        auto levelresult = level.cellTypeAtPosition(_player.tryposition);
        commitTurn(levelresult);
      }
    }

    void Player::commitTurn() {
      checkState(Const::playerstate::SAFE);

      auto _player = getPlayer();

      Level::Level level(_self, _player.idlvl);
      level.checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level
      auto _level = level.getLevel();

      auto levelresult = level.cellTypeAtPosition(_player.tryposition);
      commitTurn(levelresult);
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

    void Player::commitTurn(Const::playerstate result) {
      auto player = getPlayer();
      update(_entKey, [&](auto& p) {
        p.currentposition = player.tryposition;
        p.levelresult = result;
        p.resulttimestamp = Utils::now();
        p.triesleft = Const::retriesCount;
      });
    }

    void Player::commitTake(asset amount) {
      auto player = getPlayer();
      update(_entKey, [&](auto& p) {
        p.levelresult = Const::playerstate::TAKE;
        p.resulttimestamp = Utils::now();
        p.triesleft = Const::retriesCount;
        p.vestingbalance += amount;
      });
    }

    void Player::claimGreen() {
      checkState(Const::playerstate::GREEN);

      auto _player = getPlayer();
      resetPositionAtLevel(_player.idlvl);
    }

    void Player::claimRed() {
      checkState(Const::playerstate::RED);

      auto _player = getPlayer();
      
      Level::Level level(_self, _player.idlvl);      
      auto _level = level.getLevel();
      uint64_t idlevel = (_level.idparent > 0 ? _level.idparent : _level.id);
      resetPositionAtLevel(idlevel);
    }

    void Player::claimTake() {      
      checkState(Const::playerstate::TAKE);
      
      auto _player = getPlayer();
      
      Level::Level level(_self, _player.idlvl);
      auto _level = level.getLevel();

      BranchMeta::BranchMeta meta(_self, _level.idmeta);
      auto _meta = meta.getMeta();
      auto expiredAfter = (_player.resulttimestamp + _meta.tkintrvl) - Utils::now();
      check(
        expiredAfter <= 0,
        string("TAKE state did not expired yet. Seconds left until expiration: ") + std::to_string(expiredAfter)
      );

      resetPositionAtLevel(_player.idlvl);
      
      //Move player's vested balance to active balance
      update(_entKey, [&](auto& p) {
        p.activebalance += p.vestingbalance;
        p.vestingbalance = asset{0, Const::acceptedSymbol};
      });
    }

    void Player::resetPositionAtLevel(uint64_t idlvl) {
      update(_entKey, [&](auto& p) {
        p.idlvl = idlvl;
        p.tryposition = 0;
        p.currentposition = 0;
        p.levelresult = Const::playerstate::SAFE;
        p.resulttimestamp = 0;
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
        p.idlvl != 0,
        "First select branch to play on with action switchbrnch."
      );
    }

    void Player::checkState(Const::playerstate state) {
      auto p = getPlayer();
      checkActivePlayer();
      check(
        p.levelresult == state,
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

    void Player::checkSwitchBranchAllowed() {
      auto p = getPlayer();
      check(
        p.levelresult == Const::playerstate::INIT ||
        p.levelresult == Const::playerstate::SAFE,
        "Player can switch branch only from safe locations."
      );
    }

    void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
      auto p = getPlayer();
      check(
        p.idlvl == idlvl,
        "Player must be at previous level to unlock next one."
      );
      check(
        (
          p.levelresult == Const::playerstate::GREEN ||
          p.levelresult == Const::playerstate::TAKE
        ),
        "Player can unlock level only from GREEN position"
      );
      check(
        p.triesleft >= 1,
        "No retries left"
      );
    }

    //DEBUG only, payer == contract
    void Player::reposition(uint64_t idlevel, uint8_t position) {
      update(_self, [&](auto& p) {
        p.idlvl = idlevel;
        p.tryposition = position;
        p.triesleft = Const::retriesCount;
      });
    }
  }
} // namespace Woffler
