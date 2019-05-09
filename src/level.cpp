#include <level.hpp>

namespace Woffler {
  namespace Level {    
    Level::Level(name self, uint64_t idlevel) : 
      Entity<levels, DAO, uint64_t>(self, idlevel) {}

    DAO::DAO(levels& _levels, uint64_t idlevel): 
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, idlevel) {}

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, BranchMeta::wflbrnchmeta meta) {
      return createLevel(payer, potbalance, idbranch, meta.id, meta.lvlreds, meta.lvllength);
    }

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idmeta, uint8_t redcnt, uint8_t lvllength) {
      _entKey = nextPK();      
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idmeta = idmeta;
        l.potbalance = potbalance;
      });
      generateRedCells(payer, redcnt, lvllength);
      return _entKey;
    }

    void Level::generateRedCells(name payer, uint8_t redcnt, uint8_t lvllength) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      update(payer, [&](auto& l) {
        l.redcells = generateCells(rnd, redcnt, lvllength);
      });
    }  

    void Level::unlockTrial(name payer, uint8_t greencnt, uint8_t lvllength) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      update(payer, [&](auto& l) {
        l.greencells = generateCells(rnd, greencnt, lvllength);
        l.locked = Utils::hasIntersection<uint8_t>(l.greencells, l.redcells);
      });
    }

    void Level::addPot(name payer, asset pot) {
      update(payer, [&](auto& l) {
        l.potbalance += pot;
      });
    }

    Const::playerstate Level::cellTypeAtPosition(uint8_t position) {
      auto levelresult = Const::playerstate::SAFE;
      auto l = getEnt<wfllevel>();
      if (std::find(l.redcells.begin(), l.redcells.end(), position) != l.redcells.end()) {
        levelresult = Const::playerstate::RED;
      } else if (std::find(l.greencells.begin(), l.greencells.end(), position) != l.greencells.end()) {
        levelresult = Const::playerstate::GREEN;
      }
      return levelresult;
    }

    void Level::rmLevel() {
      checkLevel();
      remove();
    }
    
    void Level::checkLevel() {
      check(
        isEnt(),
        "Level not found."
      );            
    }

    void Level::checkLockedLevel() {
      auto l = getEnt<wfllevel>();
      check(
        l.locked,
        "Level is already unlocked."
      );
    }

    void Level::checkUnlockedLevel() {
      auto l = getEnt<wfllevel>();
      check(
        !l.locked,
        "Level is locked."
      );
    }
  }
}