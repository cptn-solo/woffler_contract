#include <woffler.hpp>

void woffler::switchbrnch(name player, uint64_t idbranch) {
  require_auth(player);

  auto self = get_self();

  //find player
  players _players(self, self.value);    
  auto _player = _players.find(player.value);
  check(
    _player != _players.end(),
    string("Account ") + player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + self.to_string() + string(" first.")
  );
  check(
    _player->levelresult == Const::playerstate::INIT ||
    _player->levelresult == Const::playerstate::SAFE,
    "Player can switch branch only from safe locations."
  );
 
  //find branch of the level
  branches _branches(self, self.value);
  const auto& _branch = _branches.find(idbranch);
  check(
    _branch->generation == 1,
    "Player can start only from root branch"
  );
  check(
    _branch->idrootlvl != 0,
    "Branch has no root level yet."
  );
 
   //check if branch is unlocked (its root level is not locked)
  levels _levels(self, self.value);
  auto _level = _levels.find(_branch->idrootlvl);
  check(
    !_level->locked,
    "Root level of the branch is locked."
  );

  //position player in root level of the branch
  _players.modify(_player, player, [&]( auto& p ) {
    p.idlvl = _level->id;
    p.triesleft = Const::retriesCount;     
    p.levelresult = Const::playerstate::SAFE;
    p.tryposition = 0;
    p.currentposition = 0;
  });

}