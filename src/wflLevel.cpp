#include <wflLevel.hpp>

Level::Level(name _self, uint64_t _idlevel) : _levels(_self, _self.value) {
  this->_self = _self;
  this->_levels = _levels;
  this->_idlevel = _idlevel;
}

template<typename Lambda>
void Level::updateState(name payer, Lambda&& updater) {
  auto level = _levels.find(_idlevel);
  _levels.modify(*level, payer, std::forward<Lambda&&>(updater)); 
}

void Level::checkLevel() {
  auto level = _levels.find(_idlevel);
  check(
    level != _levels.end(),
    "Level not found."
  );        
  idlevel = level->id;
  idparent = level->idparent;
  redcells = level->redcells;
  greencells = level->greencells;
  locked = level->locked;
  root = level->root;
  idbranch = level->idbranch;      
  idchbranch = level->idchbranch;
  idmeta = level->idmeta;
}

void Level::checkLockedLevel() {
  checkLevel();
  check(
    locked,
    "Level is already unlocked."
  );
}

void Level::checkUnlockedLevel() {
  checkLevel();
  check(
    !locked,
    "Level is locked."
  );
}

void Level::createLevel(name payer, asset pot, uint64_t branchid, uint64_t metaid, uint8_t lvlreds, uint8_t lvllength) {
  auto idlvl = Utils::nextPrimariKey(_levels.available_primary_key());

  idlevel = idlvl;
  _idlevel = idlvl;
  idbranch = branchid;
  idmeta = metaid;
  potbalance = pot;

  _levels.emplace(payer, [&](auto& l) {
    l.id = idlevel;
    l.idbranch = idbranch;
    l.idmeta = idmeta;
    l.potbalance = potbalance;
  });

  generateRedCells(payer, lvlreds, lvllength);
}

void Level::generateRedCells(name payer, uint8_t lvlreds, uint8_t lvllength) {
  auto rnd = randomizer::getInstance(payer, idlevel);
  redcells = generateCells(rnd, lvlreds, lvllength);

  updateState(payer, [&](auto& l) {
    l.redcells = redcells;
  });
}  

void Level::unlockTrial(name payer, uint8_t lvlgreens, uint8_t lvllength) {
  auto rnd = randomizer::getInstance(payer, idlevel);
  greencells = generateCells(rnd, lvlgreens, lvllength);
  locked = Utils::hasIntersection<uint8_t>(greencells, redcells);
  updateState(payer, [&](auto& l) {
    l.greencells = greencells;
    l.locked = locked;
  });
}

void Level::addPot(name payer, asset pot) {
  potbalance += pot;
  updateState(payer, [&](auto& l) {
    l.potbalance = potbalance;
  });
}

Const::playerstate Level::cellTypeAtPosition(uint8_t position) {
  auto levelresult = Const::playerstate::SAFE;
  if (std::find(redcells.begin(), redcells.end(), position) != redcells.end()) {
    levelresult = Const::playerstate::RED;
  } else if (std::find(greencells.begin(), greencells.end(), position) != greencells.end()) {
    levelresult = Const::playerstate::GREEN;
  }
  return levelresult;
}

void Level::rmLevel() {
  auto level = _levels.find(_idlevel);
  check(
    level != _levels.end(),
    "Level not found"
  );
  _levels.erase(level);
}