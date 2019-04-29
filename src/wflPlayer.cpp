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

void woffler::tryturn(name player) {
  require_auth(player);

  auto self = get_self();

  players _players(self, self.value);    
  auto _player = _players.find(player.value);

  check(
    _player != _players.end(),
    string("Account ") + player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + self.to_string() + string(" first.")
  );

  /* Checks and prerequisites */
  tryTurnChecks(*_player);

  /* Turn logic */
  //find player's current level 
  levels _levels(self, self.value);
  auto _level = _levels.find(_player->idlvl);

  //getting branch meta to decide on level presets
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(_level->idmeta);

  if (_player->triesleft >= 1) {
    //get current position and produce tryposition by generating random offset
    auto rnd = randomizer::getInstance(player, _player->idlvl);
    auto tryposition = (_player->currentposition + rnd.range(Const::tryturnMaxDistance)) % _meta->lvllength;
    
    _players.modify(_player, player, [&]( auto& p ) {
      p.tryposition = tryposition;
      p.triesleft -= 1;
    });
  }

  if (_player->triesleft == 0) {
    _players.modify(_player, player, [&]( auto& p ) {
      commitPlayersTurn(p, *_level);
    });
  }
}

void woffler::committurn(name player) {
  require_auth(player);

  auto self = get_self();

  players _players(self, self.value);    
  auto _player = _players.find(player.value);
  
  check(
    _player != _players.end(),
    string("Account ") + player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + self.to_string() + string(" first.")
  );

  /* Checks and prerequisites */
  tryTurnChecks(*_player);

  /* Turn logic */
  //find player's current level 
  levels _levels(self, self.value);
  auto _level = _levels.find(_player->idlvl);

  _players.modify(_player, player, [&]( auto& p ) {
    commitPlayersTurn(p, *_level);
  }); 
}

void woffler::claimred(name player) {
  require_auth(player);

  auto self = get_self();

  players _players(self, self.value);    
  auto _player = _players.find(player.value);
  
  check(
    _player != _players.end(),
    string("Account ") + player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + self.to_string() + string(" first.")
  );

  /* Checks and prerequisites */
  check(
    _player->idlvl != 0,
    "First select branch to play on with action switchbrnch."
  );
  check(
    _player->levelresult == Const::playerstate::RED,
    "Player position must be 'RED'."
  );

  /* Turn logic */
  //find player's current level 
  levels _levels(self, self.value);
  auto _level = _levels.find(_player->idlvl);

  _players.modify(_player, player, [&]( auto& p ) {
    if (_level->idparent != 0) {
      p.idlvl = _level->idparent;
    }
    p.tryposition = 0;
    p.currentposition = 0;
    p.levelresult = Const::playerstate::SAFE;
    p.resulttimestamp = 0;
    p.triesleft = Const::retriesCount;
  });   
}

void woffler::claimgreen(name player) {
  require_auth(player);

  auto self = get_self();

  players _players(self, self.value);    
  auto _player = _players.find(player.value);
  
  check(
    _player != _players.end(),
    string("Account ") + player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + self.to_string() + string(" first.")
  ); 

  /* Checks and prerequisites */
  check(
    _player->idlvl != 0,
    "First select branch to play on with action switchbrnch."
  );
  check(
    _player->levelresult == Const::playerstate::GREEN,
    "Player position must be 'GREEN'."
  );

  /* Claim logic */

  _players.modify(_player, player, [&]( auto& p ) {
    p.tryposition = 0;
    p.currentposition = 0;
    p.levelresult = Const::playerstate::SAFE;
    p.resulttimestamp = 0;
    p.triesleft = Const::retriesCount;
  });   
}

void woffler::claimtake(name player) {

}

void woffler::tryTurnChecks(const woffler::wflplayer& _player) {
  check(
    _player.idlvl != 0,
    "First select branch to play on with action switchbrnch."
  );
  check(
    _player.levelresult == Const::playerstate::SAFE,
    "Player can make turn only from safe locations."
  );
}

void woffler::commitPlayersTurn(woffler::wflplayer& p, const woffler::wfllevel& l) {
  p.currentposition = p.tryposition;
  if (std::find(l.redcells.begin(), l.redcells.end(), p.currentposition) != l.redcells.end()) {
    p.levelresult = Const::playerstate::RED;
  } else if (std::find(l.greencells.begin(), l.greencells.end(), p.currentposition) != l.greencells.end()) {
    p.levelresult = Const::playerstate::GREEN;
  }
  p.resulttimestamp = Utils::now();
  p.triesleft = Const::retriesCount;
}