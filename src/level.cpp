#include <level.hpp>
#include <stake.hpp>

namespace Woffler {
  namespace Level {    
    Level::Level(name self, uint64_t idlevel) : 
      Entity<levels, DAO, uint64_t>(self, idlevel) {}

    Level::Level(name self) : Level(self, 0) {}

    PlayerLevel::PlayerLevel(name self, name account) : 
      Level(self), player(self, account) {
        fetchByKey(player.getPlayer().idlvl);
    }

    DAO::DAO(levels& _levels, uint64_t idlevel): 
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, idlevel) {}

    DAO::DAO(levels& _levels, levels::const_iterator itr): 
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, itr) {}

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, BranchMeta::wflbrnchmeta meta) {
      return createLevel(payer, potbalance, idbranch, idparent, meta.id, meta.lvlreds, meta.lvllength);
    }

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, uint64_t idmeta, uint8_t redcnt, uint8_t lvllength) {
      _entKey = nextPK();      
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idparent = idparent;
        l.idmeta = idmeta;
        l.potbalance = potbalance;
      });
      generateRedCells(payer, redcnt, lvllength);
      return _entKey;
    }
    wfllevel Level::getLevel() {
      return getEnt<wfllevel>();
    } 

    void Level::unlockLevel(name owner) {
      checkLockedLevel();

      //find player
      Player::Player player(_self, owner);
      player.checkPlayer();

      auto _level = getEnt<wfllevel>();
      /* Restrictions check */
      if (_level.root) {//root level can be unlocked only by stakeholder, unlimited retries count
        //find stake to use as pot value for root level
        Stake::Stake stake(_self, 0);
        stake.checkIsStakeholder(owner, _level.idbranch);
      }
      else {//"next" level can be unlocked only from GREEN position, retries count limited
        player.checkLevelUnlockTrialAllowed(_entKey);
        player.useTry();
      }

      /* Generate cells */  

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _level.idmeta);      
      auto _meta = meta.getMeta();
      
      unlockTrial(owner, _meta.lvlgreens, _meta.lvllength);

      if (!_level.root && !_level.locked) {
        //process NEXT workflow: position player to the unlocked level
        player.resetPositionAtLevel(_level.id);
      }
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
      auto l = getLevel();
      if (std::find(l.redcells.begin(), l.redcells.end(), position) != l.redcells.end()) {
        levelresult = Const::playerstate::RED;
      } else if (std::find(l.greencells.begin(), l.greencells.end(), position) != l.greencells.end()) {
        levelresult = Const::playerstate::GREEN;
      }
      return levelresult;
    }

    void Level::regenCells(name owner) {
      auto _level = getLevel();
      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _level.idmeta);    
      auto _meta = meta.getMeta();

      generateRedCells(owner, _meta.lvlreds, _meta.lvllength);
      unlockTrial(owner, _meta.lvlgreens, _meta.lvllength);
    }

    void Level::rmLevel() {
      remove();
    }
    
    void Level::checkLevel() {
      check(
        isEnt(),
        "Level not found."
      );            
    }

    void Level::checkLockedLevel() {
      auto l = getLevel();
      check(
        l.locked,
        "Level is already unlocked."
      );
    }

    void Level::checkUnlockedLevel() {
      auto l = getLevel();
      check(
        !l.locked,
        "Level is locked."
      );    
    }

    #pragma region ** PlayerLevel **
    
    void PlayerLevel::nextLevel() {
      player.checkState(Const::playerstate::GREEN);
      
      auto _player = player.getPlayer();
      auto _curl = getLevel();
      
      auto nextidx = getIndex<"byparent"_n>();
      auto nextlitr = nextidx.find(_curl.id);

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _curl.idmeta);      
      auto _meta = meta.getMeta();

      if (nextlitr == nextidx.end()) { //create locked        
        //setting new winner of current branch
        Branch::Branch branch(_self, _curl.idbranch);
        branch.setWinner(_player.account);

        //decide on new level's pot
        asset nxtPot = (_curl.potbalance * _meta.nxtrate) / 100;
        if (nxtPot < _meta.potmin)
          nxtPot = _curl.potbalance;

        //create new level with nex pot
        Level nextL(_self);
        uint64_t nextId = nextL.createLevel(_player.account, nxtPot, _curl.idbranch, _curl.id, _meta);
        
        //cut current level's pot
        update(_player.account, [&](auto& l) {
          l.potbalance -= nxtPot;
        });
      }
      else if (nextlitr->locked) { //check for unlock & reposition player if true        
        if (_player.triesleft > 0) { //unlock trial
          player.useTry();//tries--
          auto rnd = randomizer::getInstance(_player.account, nextlitr->id);
          _idx.modify(*nextlitr, _player.account, [&](auto& l) {//updating lvl record fetched earlier
            l.greencells = generateCells(rnd, _meta.lvlgreens, _meta.lvllength);
            l.locked = Utils::hasIntersection<uint8_t>(l.greencells, l.redcells);
          });
          if (!nextlitr->locked) { //unlocked, reposition to next level
            player.resetPositionAtLevel(nextlitr->id);
          }
        }
        else { //no tries left, reset position in current level
          player.resetPositionAtLevel(_curl.id);
        }        
      }
      else {
        player.resetPositionAtLevel(nextlitr->id);
      }
    }
    
    #pragma endregion
  }
}