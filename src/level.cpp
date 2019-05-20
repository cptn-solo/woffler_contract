#include <level.hpp>
#include <stake.hpp>
#include <branch.hpp>
#include <channel.hpp>

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
      return createLevel(payer, potbalance, idbranch, idparent, meta.id, meta.lvlreds);
    }

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, uint64_t idmeta, uint8_t redcnt) {
      _entKey = nextPK();
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idparent = idparent;
        l.idmeta = idmeta;
        l.potbalance = potbalance;
      });
      generateRedCells(payer, redcnt);
      return _entKey;
    }

    wfllevel Level::getLevel() {
      return getEnt<wfllevel>();
    }

    void Level::unlockRootLevel(name owner) {
      checkRootLevel();
      checkLockedLevel();

      auto _level = getEnt<wfllevel>();
      /* Restrictions check */
      if (_level.idparent > 0) {
        //Retries count to unlock split branch levels is restricted. 
        //Additional tries are bought by calling `splitbet` action.
        Player::Player player(_self, owner);
        player.checkLevelUnlockTrialAllowed(_level.idparent);
        player.useTry();
      }

      //Root levels of both root and split branches can be unlocked only by stakeholders
      Stake::Stake stake(_self, 0);
      stake.checkIsStakeholder(owner, _level.idbranch);

      /* Generate cells */

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _level.idmeta);
      auto _meta = meta.getMeta();
      unlockTrial(owner, _meta.lvlgreens);
    }

    void Level::generateRedCells(name payer, uint8_t redcnt) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      update(payer, [&](auto& l) {
        l.redcells = generateCells(rnd, redcnt);
      });
    }

    void Level::unlockTrial(name payer, uint8_t greencnt) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      update(payer, [&](auto& l) {
        l.greencells = generateCells(rnd, greencnt);
        l.locked = Utils::hasIntersection(l.greencells, l.redcells);
      });
    }

    void Level::addPot(name payer, asset pot) {
      update(payer, [&](auto& l) {
        l.potbalance += pot;
      });
    }

    Const::playerstate Level::cellTypeAtPosition(uint8_t position) {
      check(position <= Const::lvlLength, "Position in the level can't exceed 16");

      auto levelresult = Const::playerstate::SAFE;
      auto l = getLevel();
      uint16_t pos16 = 1<<position;
      if (Utils::hasIntersection(pos16, l.redcells)) {
        levelresult = Const::playerstate::RED;
      } else if (Utils::hasIntersection(pos16, l.greencells)) {
        levelresult = Const::playerstate::GREEN;
      }
      return levelresult;
    }

    void Level::regenCells(name owner) {
      auto _level = getLevel();
      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _level.idmeta);
      auto _meta = meta.getMeta();

      generateRedCells(owner, _meta.lvlreds);
      unlockTrial(owner, _meta.lvlgreens);
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

    void Level::checkRootLevel() {
      auto l = getLevel();
      check(
        l.root,
        "Please use this action to unlock Root levels only. Root level can be unlocked only by its branch stakeholder, unlimited retries count."
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
            l.greencells = generateCells(rnd, _meta.lvlgreens);
            l.locked = Utils::hasIntersection(l.greencells, l.redcells);
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

    void PlayerLevel::takeReward() {
      player.checkState(Const::playerstate::GREEN);

      auto _player = player.getPlayer();
      auto _curl = getLevel();

      BranchMeta::BranchMeta meta(_self, _curl.idmeta);
      auto _meta = meta.getMeta();

      auto reward = (_curl.potbalance * _meta.tkrate) / 100;

      //solved * (100-TAKE_RATE)% > POT_MIN ?
      check(reward.amount > 0, "Reward amount must be > 0");
      check(
        (_curl.potbalance - reward) >= _meta.potmin,
        string("Level pot is insufficient for reward, must be at least ")+_meta.potmin.to_string().c_str()+string(" after reward paid.\n")
      );

      //Cut TAKE_RATE% of solved pot and append to winner's vesting balance
      update(_player.account, [&](auto& l) {
        l.potbalance -= reward;
      });

      //Set winner level result to TAKE and update result timestamp
      player.commitTake(reward);//retries reset, reward added to vesting

    }

    void PlayerLevel::unjailPlayer() {
      player.checkState(Const::playerstate::RED);

      auto _player = player.getPlayer();
      auto _curl = getLevel();

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _curl.idmeta);
      auto _meta = meta.getMeta();

      //calculate unjail price
      auto unjailAmount = (_curl.potbalance * _meta.unjlrate) / 100;
      if (unjailAmount < _meta.unjlmin)
        unjailAmount = _meta.unjlmin;

      // print("Un-jail amount: ", asset{unjailAmount}, "\n");

      //cut player's active balance with unjail payment value
      player.subBalance(unjailAmount, _player.account);//will fail if balance not cover amount being cut

      //calculate revenue share
      auto revshareAmount = (unjailAmount * _meta.unjlrate) / 100;
      // print("Rev.share amount: ", asset{revshareAmount}, "\n");

      //cut unjail vaule with revenue share
      unjailAmount -= revshareAmount;

      //put revenue share into branch stakeholders' revenue (!defer, recursion to parent branches)
      Branch::Branch branch(_self, _curl.idbranch);
      branch.deferRevenueShare(revshareAmount);//branch winner will get some here, all winners along branch hierarchy

      //calculate sales channel fee for remaining unjail value
      Channel::Channel channel(_self, _player.channel);
      auto channelAmount = (revshareAmount * (channel.getRate() + _meta.slsrate)) / 100;

      // print("Referrer amount: ", asset{channelAmount}, "\n");

      //cut unjail vaule with sales channel fee
      unjailAmount -= channelAmount;

      //put sales channel fee into sales channel balance (!defer)
      channel.addBalance(channelAmount, _self);

      // print("Pot balance before add: ", asset{_curl.potbalance}, "\n");

      //put remaining unjail payment into level's pot
      update(_player.account, [&](auto& l) {
        l.potbalance += unjailAmount;
      });

      // print("Added to the pot: ", asset{unjailAmount}, "\n");

      //reset player's state, retries and position in current level's zero cell
      player.resetPositionAtLevel(_curl.id);
    }

    void PlayerLevel::splitLevel() {
      //Player's level result is "GREEN"?
      player.checkState(Const::playerstate::GREEN);

      auto _player = player.getPlayer();
      auto _curl = getLevel();

      //getting branch meta to decide on level presets
      BranchMeta::BranchMeta meta(_self, _curl.idmeta);
      auto _meta = meta.getMeta();

      //Child branch exists for winner's current level?
      check(_curl.idchbranch == 0, "Branch already split at the current level");

      //solved * SPLIT_RATE% * STAKE_RATE% > STAKE_MIN?
      auto minSplitAmount = (_meta.stkmin * 100) / _meta.stkrate;
      auto mitSplittablePotAmount = (minSplitAmount * 100) / _meta.spltrate;
      check(
        _curl.potbalance >= mitSplittablePotAmount,
        string("Reward pot of the current level is too small, should be at least ") + mitSplittablePotAmount.to_string().c_str()
      );

      auto splitAmount = (_curl.potbalance * _meta.spltrate) / 100;

      Branch::Branch branch(_self, 0);
      //Create child branch and a locked level with "Red" cells, branch generation++
      //Add bet price to player's branch stake
      auto betPrice = (splitAmount * _meta.stkrate) / 100;
      uint64_t idchbranch = branch.createChildBranch(_player.account, _meta, betPrice, _curl.idbranch);
      //Move SPLIT_RATE% of solved pot to locked pot
      Level nextL(_self);
      uint64_t idlevel = nextL.createLevel(_player.account, splitAmount, idchbranch, _curl.id, _meta);
      branch.setRootLevel(_player.account, idlevel);

      update(_player.account, [&](auto& l) {
        l.potbalance -= splitAmount;
        l.idchbranch = idchbranch;
      });

      //Reset retries count
      player.resetRetriesCount();
    }

    #pragma endregion
  }
}