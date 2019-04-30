#include <wflPlayer.hpp>

namespace woffler {
  namespace wflPlayer {
    Player::Player(name _self, name _player) : _players(_self, _self.value) {
      this->_self = _self;
      this->_player = _player;
      this->_players = _players;
    }

    template<typename Lambda>
    void Player::updateState(name payer, Lambda&& updater) {
      auto player = _players.find(_player.value);
      _players.modify(*player, payer, std::forward<Lambda&&>(updater)); 
    }

    void Player::checkPlayer() {
      auto player = _players.find(_player.value);
      check(
        player != _players.end(),
        string("Account ") + _player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
      ); 
      idlevel = player->idlvl;
      levelresult = player->levelresult;
      tryposition = player->tryposition;
      triesleft = player->triesleft;
      activebalance = player->activebalance;
    }
    
    void Player::checkNoPlayer() {
      auto player = _players.find(_player.value);
      check(
        player == _players.end(), 
        "Account already exists"
      );
    }

    void Player::checkActivePlayer() {
      checkPlayer();
      check(
        idlevel != 0,
        "First select branch to play on with action switchbrnch."
      );
    }

    void Player::checkState(Const::playerstate state) {
      checkActivePlayer();    
      check(
        levelresult == state,
        string("Player current level resutl must be '") + std::to_string(state) + string("'.")
      );
    }

    void Player::checkBalanceCovers(asset amount) {
      checkPlayer();
      check(
        activebalance >= amount, 
        string("Not enough active balance in your account. Current active balance: ") + activebalance.to_string().c_str() 
      );    
    }

    void Player::checkBalanceZero() {
      checkPlayer();
      check(//warning! works only for records, emplaced in contract's host scope
        activebalance == asset{0, Const::acceptedSymbol},
        string("Please withdraw funds first. Current active balance: ") + activebalance.to_string().c_str()
      );
    }

    void Player::checkSwitchBranchAllowed() {
      checkPlayer();
      check(
        levelresult == Const::playerstate::INIT ||
        levelresult == Const::playerstate::SAFE,
        "Player can switch branch only from safe locations."
      );
    }

    void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
      check(
        idlevel == idlvl,
        "Player must be at previous level to unlock next one."
      );
      check(
        (
          levelresult == Const::playerstate::GREEN || 
          levelresult == Const::playerstate::TAKE
        ),
        "Player can unlock level only from GREEN position"
      );
      check(
        triesleft >= 1,
        "No retries left"
      );
    }
    
    void Player::createPlayer(name achannel) {
      channel = achannel;
      _players.emplace(_player, [&](auto& p) {
        p.account = _player;
        p.channel = channel;
      });
    }

    bool Player::addBalance(asset amount, name payer) {    
      auto player = _players.find(_player.value);    
      if (player == _players.end()) 
        return false;//without exception, bc it is allowed while signing up
      
      _players.modify(*player, payer, [&]( auto& p ) {
        p.activebalance += amount;     
      });
      activebalance = player->activebalance;
      
      print("Current balance: ", asset{activebalance});
      
      return true;  
    }

    void Player::subBalance(asset amount, name payer) {
      checkBalanceCovers(amount);
      activebalance -= amount;
      updateState(payer, [&]( auto& p ) {
        p.activebalance = activebalance;     
      });     
    }

    bool Player::rmAccount() {
      checkBalanceZero();
      auto player = _players.find(_player.value);
      _players.erase(player);
      
      print("Removed user: ", name{_player}, " from scope: ", name{_self});
      
      return true;
    }

    void Player::switchRootLevel(uint64_t idlvl) {
      //position player in root level of the branch
      idlevel = idlvl;
      triesleft = Const::retriesCount;
      levelresult = Const::playerstate::SAFE;
      tryposition = 0;
      currentposition = 0;

      updateState(_player, [&](auto& p) {
        p.idlvl = idlevel;
        p.triesleft = triesleft;     
        p.levelresult = levelresult;
        p.tryposition = tryposition;
        p.currentposition = currentposition;
      });
    }

    void Player::useTry() {
      useTry(tryposition);  
    }

    void Player::useTry(uint8_t position) {
      tryposition = position;
      triesleft -= 1;

      updateState(_player, [&](auto& p) {
        p.tryposition = tryposition;
        p.triesleft = triesleft;
      });
    }

    void Player::commitTurn(Const::playerstate result) {
      currentposition = tryposition;
      levelresult = result;
      triesleft = Const::retriesCount;

      updateState(_player, [&](auto& p) {
        p.currentposition = tryposition;
        p.levelresult = levelresult;
        p.resulttimestamp = Utils::now();
        p.triesleft = triesleft;
      });
    }

    void Player::resetPositionAtLevel(uint64_t idlvl) {
      idlevel = idlvl;
      tryposition = 0;
      currentposition = 0;
      levelresult = Const::playerstate::SAFE;
      triesleft = Const::retriesCount;

      updateState(_player, [&](auto& p) {
        p.idlvl = idlevel;
        p.tryposition = tryposition;
        p.currentposition = currentposition;
        p.levelresult = levelresult;
        p.resulttimestamp = 0;
        p.triesleft = triesleft;
      });
    }
  }
}
