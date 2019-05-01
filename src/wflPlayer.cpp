#include <wflPlayer.hpp>

Player::Player(name _self, name _player) : _players(_self, _self.value) {
  this->_self = _self;
  this->_player = _player;
  this->_players = _players;
  this->player = &_players.find(_player.value);
}

template<typename Lambda>
void Player::updateState(name payer, Lambda&& updater) {
  _players.modify(*player, payer, std::forward<Lambda&&>(updater)); 
}

bool Player::isRegistred() {      
  return (*player != _players.end());
}

void Player::checkPlayer() {
  check(
    *player != _players.end(),
    string("Account ") + _player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
  ); 
}

void Player::checkNoPlayer() {      
  check(
    *player == _players.end(), 
    "Account already exists"
  );
}

void Player::checkActivePlayer() {
  checkPlayer();
  auto p = *player;
  check(
    p->idlvl != 0,
    "First select branch to play on with action switchbrnch."
  );
}

void Player::checkState(Const::playerstate state) {
  checkActivePlayer();    
  auto p = *player;
  check(
    p->levelresult == state,
    string("Player current level resutl must be '") + std::to_string(state) + string("'.")
  );
}

void Player::checkBalanceCovers(asset amount) {
  checkPlayer();
  auto p = *player;
  check(
    p->activebalance >= amount, 
    string("Not enough active balance in your account. Current active balance: ") + p->activebalance.to_string().c_str() 
  );    
}

void Player::checkBalanceZero() {
  checkPlayer();
  auto p = *player;
  check(//warning! works only for records, emplaced in contract's host scope
    p->activebalance == asset{0, Const::acceptedSymbol},
    string("Please withdraw funds first. Current active balance: ") + p->activebalance.to_string().c_str()
  );
}

void Player::checkSwitchBranchAllowed() {
  checkPlayer();
  auto p = *player;
  check(
    p->levelresult == Const::playerstate::INIT ||
    p->levelresult == Const::playerstate::SAFE,
    "Player can switch branch only from safe locations."
  );
}

void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
  auto p = *player;
  check(
    p->idlvl == idlvl,
    "Player must be at previous level to unlock next one."
  );
  check(
    (
      p->levelresult == Const::playerstate::GREEN || 
      p->levelresult == Const::playerstate::TAKE
    ),
    "Player can unlock level only from GREEN position"
  );
  check(
    p->triesleft >= 1,
    "No retries left"
  );
}

void Player::createPlayer(name channel, name payer) {
  player = &_players.emplace(payer, [&](auto& p) {
    p.account = _player;
    p.channel = channel;
  });
}

void Player::addBalance(asset amount, name payer) {    
  checkPlayer();      
  auto p = *player;
  _players.modify(p, payer, [&](auto& p) {
    p.activebalance += amount;     
  });      
  print("Current balance: ", asset{p->activebalance});      
}

void Player::subBalance(asset amount, name payer) {
  checkBalanceCovers(amount);
  updateState(payer, [&]( auto& p ) {
    p.activebalance -= amount;     
  });     
  auto p = *player;
  print("Current balance: ", asset{p->activebalance});      
}

void Player::rmAccount() {
  checkBalanceZero();
  _players.erase(*player);      
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
  auto p = *player;
  useTry(p->tryposition);  
}

void Player::useTry(uint8_t position) {
  updateState(_player, [&](auto& p) {
    p.tryposition = position;
    p.triesleft -= 1;
  });
}

void Player::commitTurn(Const::playerstate result) {
  updateState(_player, [&](auto& p) {
    p.currentposition = (*player)->tryposition;
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
