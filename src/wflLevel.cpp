#include <woffler.hpp>

uint64_t woffler::addLevel(name owner, const wflbranch& branch) {
  auto self = get_self();
  
  //find stake to use as pot value for root level
  stakes _stakes(self, self.value);

  //calculating branch stake total (all stakeholders)
  auto stkidx = _stakes.get_index<name("bybranch")>();
  auto stkitr = stkidx.lower_bound(branch.id);
  auto branchStake = asset{0, Const::acceptedSymbol};
  while(stkitr != stkidx.end()) {
    branchStake+=stkitr->stake;
    stkitr++;
  }

  //getting branch meta to decide on level presets
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(branch.idmeta);
  check(
    _meta != _metas.end(),
    "No branch metadata found for branch"
  );

  //emplacing new (root) level
  levels _levels(self, self.value);
  auto idlevel = Utils::nextPrimariKey(_levels.available_primary_key());
  auto rnd = randomizer::getInstance(owner, idlevel);

  _levels.emplace(owner, [&](auto& l) {
    l.id = idlevel;
    l.idbranch = branch.id;
    l.potbalance = branchStake;
    l.redcells = generateCells(rnd, _meta->lvlreds, _meta->lvllength);
  });

  print("Root level created with id: ", std::to_string(idlevel) , ", pot balance: ", asset{branchStake}, " for branch: ", std::to_string(branch.id));    
  
  return idlevel;
}

void woffler::unlocklvl(name owner, uint64_t idlevel) {
  require_auth(owner);
  auto self = get_self();
  
  /* Getting into context */

  //find level to unlock
  levels _levels(self, self.value);
  auto _level = _levels.find(idlevel);
  check(
    _level != _levels.end(),
    "No level found."
  );
  check(
    _level->locked,
    "Level is not locked"
  );

  //find player
  players _players(self, self.value);    
  auto _player = _players.find(owner.value);
  check(
    _player != _players.end(),
    "Player not found"
  );

  //find branch of the level
  branches _branches(self, self.value);
  const auto& _branch = _branches.find(_level->idbranch);
  
  bool _nextLevelUnlock = false;
  
  /* Restrictions check */
  if (_branch->idrootlvl == idlevel) {//root level can be unlocked only by stakeholder, unlimited retries count
    //find stake to use as pot value for root level
    stakes _stakes(self, self.value);

    auto ownedBranchId = Utils::combineIds(owner.value, _level->idbranch);    
    auto stkidx = _stakes.get_index<name("byownedbrnch")>();
    const auto& stake = stkidx.find(ownedBranchId);          

    check(
      stake != stkidx.end(),
      "Only stakeholder of a branch can unlock root level for it"
    );
  }
  else {//"next" level can be unlocked only from GREEN position, retries count limited
    check(
      _player->idlvl == _level->idparent,
      "Player must be at previous level to unlock next one."
    );
    check(
      _player->levelresult == Const::playerstate::GREEN,
      "Player can unlock level only from GREEN position"
    );
    check(
      _player->triesleft >= 1,
      "No retries left"
    );
    _nextLevelUnlock = true;

    _players.modify(_player, owner, [&]( auto& p ) {
      p.triesleft -= 1;     
    });
  }

  /* Generate cells */  

  //getting branch meta to decide on level presets
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(_branch->idmeta);
  auto rnd = randomizer::getInstance(owner, idlevel);

  _levels.modify(_level, owner, [&](auto& l) {
    l.greencells = generateCells(rnd, _meta->lvlgreens, _meta->lvllength);;
    l.locked = Utils::hasIntersection<uint8_t>(l.greencells, l.redcells);
  });

  if (_nextLevelUnlock && !_level->locked) {
    //process NEXT workflow: position player to the unlocked level
    _players.modify(_player, owner, [&]( auto& p ) {
      p.triesleft = Const::retriesCount;     
      p.idlvl = _level->id;
      p.tryposition = 0;
      p.currentposition = 0;
      p.levelresult = Const::playerstate::SAFE;
    });
  }
}


void woffler::addPot(name owner, uint64_t idlevel, asset pot) {
  auto self = get_self();
  levels _levels(self, self.value);
  auto _level = _levels.find(idlevel);
  check(
    _level != _levels.end(),
    "Level not found."
  );

  _levels.modify(_level, owner, [&](auto& l){
    l.potbalance += pot;
  });
  print("Value added to the pot: ", asset{pot}, ", current pot value: ", asset{_level->potbalance});  
}

template<class T>
std::vector<T> woffler::generateCells(randomizer& rnd, T size, T maxval) {

  std::vector<T> data(size);
  Cell::generator<T> generator(rnd, maxval, size);
  std::generate(data.begin(), data.end(), generator);

  return data;
}

//DEBUG: testing cells generation for a given level and meta
void woffler::regencells(name owner, uint64_t idlevel, uint64_t idmeta) {
  require_auth(owner);
  auto self = get_self();
  check(
    owner == self,
    string("Debug mode available only to contract owner: ") + self.to_string()
  );

  //getting branch meta to decide on level presets
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(idmeta);
  check(
    _meta != _metas.end(),
    "No branch metadata found."
  );
  levels _levels(self, self.value);
  auto _level = _levels.find(idlevel);
  check(
    _level != _levels.end(),
    "No level found."
  );
  auto rnd = randomizer::getInstance(owner, idlevel);
  std::vector<uint8_t> greencells = generateCells(rnd, _meta->lvlgreens, _meta->lvllength);
  std::vector<uint8_t> redcells = generateCells(rnd, _meta->lvlreds, _meta->lvllength);

  _levels.modify(_level, owner, [&](auto& l) {
    l.redcells = redcells;
    l.greencells = greencells;
    l.locked = Utils::hasIntersection<uint8_t>(greencells, redcells);
  });
}

//DEBUG: testing cell randomizer
void woffler::gencells(name account, uint8_t size, uint8_t maxval) {
  require_auth(account);
  auto self = get_self();
  check(
    account == self,
    string("Debug mode available only to contract owner: ") + self.to_string()
  );

  auto rnd = randomizer::getInstance(account, 1);
  auto data = generateCells<uint8_t>(rnd, size, maxval);
  Utils::printVectorInt<uint8_t>(data);
}

//DEBUG: testing level delete
void woffler::rmlevel(name owner, uint64_t idlevel) {
  require_auth(owner);
  auto self = get_self();
  check(
    owner == self,
    string("Debug mode available only to contract owner: ") + self.to_string()
  );

  levels _levels(self, self.value);
  auto _level = _levels.find(idlevel);
  check(
    _level != _levels.end(),
    "No level found."
  );

  _levels.erase(_level);
}
