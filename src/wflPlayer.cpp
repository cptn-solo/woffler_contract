#include <wflPlayer.hpp>

namespace woffler {
  namespace wflPlayer {
    Player::Player(name _self, name _player) : _players(_self, _self.value) {
      this->_self = _self;
      this->_player = _player;
      this->_players = _players;
      
      this->player = _players.find(_player.value);
    }

    template<typename Lambda>
    void Player::updateState(name payer, Lambda&& updater) {
      _players.modify(player, payer, std::forward<Lambda&&>(updater)); 
    }

    bool Player::isRegistred() {      
      return (player != _players.end());
    }

    void Player::checkPlayer() {
      check(
        player != _players.end(),
        string("Account ") + _player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
      ); 
    }
    
    void Player::checkNoPlayer() {      
      check(
        player == _players.end(), 
        "Account already exists"
      );
    }

    void Player::checkActivePlayer() {
      checkPlayer();
      check(
        player->idlvl != 0,
        "First select branch to play on with action switchbrnch."
      );
    }

    void Player::checkState(Const::playerstate state) {
      checkActivePlayer();    
      check(
        player->levelresult == state,
        string("Player current level resutl must be '") + std::to_string(state) + string("'.")
      );
    }

    void Player::checkBalanceCovers(asset amount) {
      checkPlayer();
      check(
        player->activebalance >= amount, 
        string("Not enough active balance in your account. Current active balance: ") + player->activebalance.to_string().c_str() 
      );    
    }

    void Player::checkBalanceZero() {
      checkPlayer();
      check(//warning! works only for records, emplaced in contract's host scope
        player->activebalance == asset{0, Const::acceptedSymbol},
        string("Please withdraw funds first. Current active balance: ") + player->activebalance.to_string().c_str()
      );
    }

    void Player::checkSwitchBranchAllowed() {
      checkPlayer();
      check(
        player->levelresult == Const::playerstate::INIT ||
        player->levelresult == Const::playerstate::SAFE,
        "Player can switch branch only from safe locations."
      );
    }

    void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
      check(
        player->idlvl == idlvl,
        "Player must be at previous level to unlock next one."
      );
      check(
        (
          player->levelresult == Const::playerstate::GREEN || 
          player->levelresult == Const::playerstate::TAKE
        ),
        "Player can unlock level only from GREEN position"
      );
      check(
        player->triesleft >= 1,
        "No retries left"
      );
    }
    
    void Player::createPlayer(name achannel, name payer) {
      player = _players.emplace(payer, [&](auto& p) {
        p.account = _player;
        p.channel = channel;
      });
    }

    void Player::addBalance(asset amount, name payer) {    
      checkPlayer();      
      _players.modify(player, payer, [&](auto& p) {
        p.activebalance += amount;     
      });      
      print("Current balance: ", asset{player->activebalance});      
    }

    void Player::subBalance(asset amount, name payer) {
      checkBalanceCovers(amount);
      updateState(payer, [&]( auto& p ) {
        p.activebalance -= amount;     
      });     
      print("Current balance: ", asset{player->activebalance});      
    }

    void Player::rmAccount() {
      checkBalanceZero();
      _players.erase(player);      
      player = NULL;
      print("Removed user: ", name{_player}, " from scope: ", name{_self});
    }

    void Player::switchRootLevel(uint64_t idlvl) {
      //position player in root level of the branch
      updateState(_player, [&](auto& p) {
        p.idlvl = idlvl;
        p.triesleft = Const::retriesCount;     
        p.levelresult = Const::playerstate::SAFE;
        p.tryposition = 0;
        p.currentposition = 0;
      });
    }

    void Player::useTry() {
      useTry(tryposition);  
    }

    void Player::useTry(uint8_t position) {
      updateState(_player, [&](auto& p) {
        p.tryposition = position;
        p.triesleft -= 1;
      });
    }

    void Player::commitTurn(Const::playerstate result) {
      updateState(_player, [&](auto& p) {
        p.currentposition = player->tryposition;
        p.levelresult = result;
        p.resulttimestamp = Utils::now();
        p.triesleft = Const::retriesCount;
      });
    }

    void Player::resetPositionAtLevel(uint64_t idlvl) {
      updateState(_player, [&](auto& p) {
        p.idlvl = idlvl;
        p.tryposition = 0;
        p.currentposition = 0;
        p.levelresult = Const::playerstate::SAFE;
        p.resulttimestamp = 0;
        p.triesleft = Const::retriesCount;
      });
    }
  }
}
