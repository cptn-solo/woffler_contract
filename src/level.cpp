#include <level.hpp>
#include <stake.hpp>
#include <branch.hpp>
#include <channel.hpp>

namespace Woffler {
  namespace Level {
    Level::Level(name self, uint64_t idlevel) :
      Entity<levels, DAO, uint64_t>(self, idlevel), meta(self, 0) {
        if (idlevel > 0)
          meta.fetchByKey(getLevel().idmeta);
      }

    Level::Level(name self) : Level(self, 0) {}

    PlayerLevel::PlayerLevel(name self, name account) :
      Level(self), player(self, account) {
        fetchByKey(player.getPlayer().idlvl);
    }

    DAO::DAO(levels& _levels, uint64_t idlevel):
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, idlevel) {}

    DAO::DAO(levels& _levels, levels::const_iterator itr):
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, itr) {}

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, uint64_t idmeta) {
      meta.fetchByKey(idmeta);
      _entKey = nextPK();
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idparent = idparent;
        l.idmeta = idmeta;
        l.potbalance = potbalance;
      });
      generateRedCells(payer);
      return _entKey;
    }

    wfllevel Level::getLevel() {
      return getEnt<wfllevel>();
    }

    void Level::unlockLevel(name owner) {
      checkLockedLevel();

      auto _level = getLevel();
      /* Restrictions check */
      if (_level.idparent != 0) {
        //Retries count to unlock split and next levels is restricted. 
        //Additional tries are bought by calling `splitbet` action fpr split levels
        Player::Player player(_self, owner);
        player.checkLevelUnlockTrialAllowed(_level.idparent);
        player.useTry();
      } 
      else {
        //Root levels of root branches can be unlocked only by stakeholders, no retries restriction
        Stake::Stake stake(_self, 0);
        stake.checkIsStakeholder(owner, _level.idbranch);
      }

      /* Generate cells */
      //getting branch meta to decide on level presets
      unlockTrial(owner);
    }

    void Level::generateRedCells(name payer) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      auto redcnt = meta.getMeta().lvlreds;
      update(payer, [&](auto& l) {
        l.redcells = generateCells(rnd, redcnt);
      });
    }

    void Level::unlockTrial(name payer) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      auto greencnt = meta.getMeta().lvlgreens;
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
      generateRedCells(owner);
      unlockTrial(owner);
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
      if (nextlitr == nextidx.end()) { //create locked
        //setting new winner of current branch
        Branch::Branch branch(_self, _curl.idbranch);
        branch.setWinner(_player.account);

        //decide on new level's pot
        asset nxtPot = meta.nextPot(_curl.potbalance);

        //create new level with nex pot
        Level nextL(_self);
        uint64_t nextId = nextL.createLevel(_player.account, nxtPot, _curl.idbranch, _curl.id, _curl.idmeta);

        //cut current level's pot
        update(_player.account, [&](auto& l) {
          l.potbalance -= nxtPot;
        });
      }
      else {
        check(!nextlitr->locked, "Next level locked. Please unlock it first using 'unlocklvl' action.");
        player.resetPositionAtLevel(nextlitr->id);
      }
    }

    void PlayerLevel::takeReward() {
      player.checkState(Const::playerstate::GREEN);

      auto _player = player.getPlayer();
      auto _curl = getLevel();

      auto reward = meta.takeAmount(_curl.potbalance);

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

      //calculate unjail price
      auto unjailPrice = meta.unjailPrice(_curl.potbalance);

      //cut player's active balance with unjail payment value
      player.subBalance(unjailPrice, _player.account);//will fail if balance not cover amount being cut

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(unjailPrice, Const::revenuetype::UNJAIL);      

      //put remaining unjail payment into level's pot
      update(_player.account, [&](auto& l) {
        l.potbalance += unjailPrice;
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

      //Child branch exists for winner's current level?
      check(_curl.idchbranch == 0, "Branch already split at the current level");      

      //getting branch meta to decide on level presets
      auto splitAmount = meta.splitPot(_curl.potbalance);

      Branch::Branch branch(_self, 0);
      //Create child branch and a locked level with "Red" cells, branch generation++
      //Add bet price to player's branch stake
      auto betPrice = meta.splitBetPrice(splitAmount);

      uint64_t idchbranch = branch.createChildBranch(_player.account, betPrice, _curl.idbranch);
      //Move SPLIT_RATE% of solved pot to locked pot
      Level nextL(_self);
      uint64_t idlevel = nextL.createLevel(_player.account, splitAmount, idchbranch, _curl.id, _curl.idmeta);
      branch.setRootLevel(_player.account, idlevel);

      update(_player.account, [&](auto& l) {
        l.potbalance -= splitAmount;
        l.idchbranch = idchbranch;
      });

      //Reset retries count
      player.resetRetriesCount();
    }
    
    void PlayerLevel::splitBet() {
      //green, locked, 0 tries
      player.checkState(Const::playerstate::GREEN);
      auto _curl = getLevel();

      check(_curl.idchbranch != 0, "Current level has no child branch for split bets.");

      auto _player = player.getPlayer();
      check(_player.triesleft == 0, "Retries count must be 0.");

      Branch::Branch chbranch(_self, _curl.idchbranch);
      auto _chbranch = chbranch.getBranch();

      Level chlevel(_self, _chbranch.idrootlvl);
      chlevel.checkLockedLevel();

      auto _chlevel = chlevel.getLevel();

      //Player's balance covers bet price? (br.meta: stkrate, stkmin)
      //Cut player's balance with bet price 
      auto betPrice = meta.splitBetPrice(_chlevel.potbalance);      
      player.subBalance(betPrice, _player.account);

      //Add bet price to player's stake in the split branch 
      chbranch.appendStake(_player.account, betPrice);

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(betPrice, Const::revenuetype::SPLITBET);      

      //Add (bet amount - rev.share) to the locked level's pot
      chlevel.addPot(_player.account, betPrice);

      //Reset retries count (3 left)
      player.resetRetriesCount();
    }

    void PlayerLevel::cutRevenueShare(asset& revenue, const Const::revenuetype& revtype) {

      //shared amount proportinal to the payment itself - so same methods applied:
      asset revenueShare = (revtype == Const::revenuetype::SPLITBET 
        ? meta.splitBetPrice(revenue)
        : meta.unjailPrice(revenue)
      );

      Channel::Channel channel(_self, player.getChannel());
      Branch::Branch branch(_self, getLevel().idbranch);//Note: CURRENT branch gets rev.share from splitbet action

      //calculate sales channel share
      auto channelShare = (revenueShare * (channel.getRate() + meta.getMeta().slsrate)) / 100;
      auto branchShare = revenueShare - channelShare;

      //put sales channel fee into sales channel balance (!defer)
      channel.addBalance(channelShare, _self);

      //put revenue share into branch stakeholders' revenue (!defer, recursion to parent branches)
      branch.deferRevenueShare(branchShare);//branch winner will get some here, all winners along branch hierarchy

      revenue -= revenueShare;
    }

    #pragma endregion
  }
}