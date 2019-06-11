#include <level.hpp>
#include <stake.hpp>
#include <branch.hpp>
#include <channel.hpp>

namespace Woffler {
  namespace Level {
    Level::Level(name self, uint64_t idlevel) :
      Entity<levels, DAO, uint64_t>(self, idlevel), meta(self, 0) {
        if (idlevel != 0)
          meta.fetchByKey(getLevel().idmeta);
      }

    Level::Level(name self) : Level(self, 0) {}

    PlayerLevel::PlayerLevel(name self, name account) :
      Level(self), player(self, account) {
        fetchByKey(player.getPlayer().idlevel);
        if (_entKey != 0)
          meta.fetchByKey(getLevel().idmeta);
    }

    DAO::DAO(levels& _levels, uint64_t idlevel):
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, idlevel) {}

    DAO::DAO(levels& _levels, levels::const_iterator itr):
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, itr) {}

    uint64_t Level::createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, uint64_t generation, uint64_t idmeta) {
      meta.fetchByKey(idmeta);
      _entKey = nextPK();
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idparent = idparent;
        l.generation = generation;
        l.idmeta = idmeta;
        l.potbalance = potbalance;
      });
      generateRedCells(payer);
      return _entKey;
    }

    wfllevel Level::getLevel() {
      return getEnt<wfllevel>();
    }

    void Level::unlockLevel(name account) {
      checkLockedLevel();

      auto _level = getLevel();
      Player::Player player(_self, account);

      /* Restrictions check */
      if (_level.idparent != 0) {
        //Retries count to unlock split and next levels is restricted. 
        //Additional tries are bought by calling `buytries` action
        player.checkLevelUnlockTrialAllowed(_level.idparent);
        player.useTry();
      } 
      else {
        //Root levels of root branches can be unlocked only by stakeholders, no retries restriction
        Stake::Stake stake(_self, 0);
        stake.checkIsStakeholder(account, _level.idbranch);
      }

      /* Generate cells */
      //getting branch meta to decide on level presets
      if (unlockTrial(account)) {
        switch (player.getPlayer().status)
        {
        case Const::playerstate::NEXT: {
          /* set winner */
          Branch::Branch branch(_self, _level.idbranch);
          branch.setWinner(account);
          player.resetPositionAtLevel(_level.id);
          break;
        }
        case Const::playerstate::SPLIT: {
          /* set stakeholder */
          Branch::Branch branch(_self, _level.idbranch);
          branch.appendStake(account, _level.potbalance);
          player.resetPositionAtLevel(_level.id);
          break;
        }
        default:
          break;
        }
      }
    }

    void Level::generateRedCells(name payer) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      auto redcnt = meta.getMeta().lvlreds;
      update(payer, [&](auto& l) {
        l.redcells = generateCells(rnd, redcnt);
      });
    }

    bool Level::unlockTrial(name payer) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      auto greencnt = meta.getMeta().lvlgreens;
      auto locked = true;
      update(payer, [&](auto& l) {
        l.greencells = generateCells(rnd, greencnt);
        l.locked = Utils::hasIntersection(l.greencells, l.redcells);
        locked = l.locked;
      });
      return !locked;
    }

    void Level::addPot(name payer, asset pot) {
      update(payer, [&](auto& l) {
        l.potbalance += pot;
      });
    }

    Const::playerstate Level::cellTypeAtPosition(uint8_t position) {
      check(position <= Const::lvlLength, "Position in the level can't exceed 16");

      auto status = Const::playerstate::SAFE;
      auto l = getLevel();
      uint16_t pos16 = 1<<position;
      if (Utils::hasIntersection(pos16, l.redcells)) {
        status = Const::playerstate::RED;
      } else if (Utils::hasIntersection(pos16, l.greencells)) {
        status = Const::playerstate::GREEN;
      }
      return status;
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
      
      while (nextlitr != nextidx.end() && nextlitr->idbranch != _curl.idbranch)
        nextlitr++;

      //getting branch meta to decide on level presets
      if (nextlitr == nextidx.end()) { //create locked
        //decide on new level's pot
        const asset nxtPot = meta.nextPot(_curl.potbalance);

        //create new level with nex pot
        Level nextL(_self);
        const uint64_t nextgen = _curl.generation + 1;
        const uint64_t nextId = nextL.createLevel(_player.account, nxtPot, _curl.idbranch, _curl.id, nextgen, _curl.idmeta);
        //setting new winner of current branch
        Branch::Branch branch(_self, _curl.idbranch);
        branch.updateTreeDept(_player.account, nextId, nextgen);

        //cut current level's pot
        update(_player.account, [&](auto& l) {
          l.potbalance -= nxtPot;
        });
        player.commitTurn(Const::playerstate::NEXT);
      }
      else if (nextlitr->locked) {
        player.commitTurn(Const::playerstate::NEXT);
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
      player.commitTake(reward, Utils::now() + meta.getMeta().tkintrvl);//retries reset, reward added to vesting
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

      //reset player's state, retries and position in current level's zero cell
      player.resetPositionAtLevel(_curl.id);
    }

    void PlayerLevel::splitLevel() {
      //Player's level result is "GREEN"?
      player.checkState(Const::playerstate::GREEN);

      auto _player = player.getPlayer();
      auto _curl = getLevel();

      auto splitidx = getIndex<"byparent"_n>();
      auto splitlitr = splitidx.find(_curl.id);
      
      while (splitlitr != splitidx.end() && splitlitr->idbranch == _curl.idbranch)
        splitlitr++;

      //Child branch exists for winner's current level?
      if (splitlitr == splitidx.end()) {
        //getting branch meta to decide on level presets
        const asset splitPot = meta.splitPot(_curl.potbalance);

        Branch::Branch branch(_self, 0);
        //Create child branch and a locked level with "Red" cells, branch generation++

        const uint64_t idchbranch = branch.createChildBranch(_player.account, _curl.idbranch);

        //Move SPLIT_RATE% of solved pot to locked pot
        Level nextL(_self);
        const uint64_t nextgen = _curl.generation+1;
        const uint64_t idlevel = nextL.createLevel(_player.account, splitPot, idchbranch, _curl.id, nextgen, _curl.idmeta);        
        branch.setRootLevel(_player.account, idlevel, nextgen);        

        update(_player.account, [&](auto& l) {
          l.potbalance -= splitPot;
          l.idchbranch = idchbranch;
        });
        player.commitTurn(Const::playerstate::SPLIT);
      } 
      else if (splitlitr->locked) {
        player.commitTurn(Const::playerstate::SPLIT);
      } 
      else {
        check(!splitlitr->locked, "Split level locked. Please unlock it first using 'unlocklvl' action.");
        player.resetPositionAtLevel(splitlitr->id);
      }
    }
    
    void PlayerLevel::buyRetries() {
      auto _player = player.getPlayer();
      check(_player.triesleft == 0, "Retries count must be 0.");
      check(
        _player.status == Const::playerstate::NEXT ||
        _player.status == Const::playerstate::SPLIT,
        "Extra retries are available only for next/split level unlock action."
      );

      auto _curl = getLevel();
      
      //Player's balance covers price? 
      //Cut player's balance with price 
      auto price = meta.buytryPrice(_curl.potbalance);
      player.subBalance(price, _player.account);

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(price, Const::revenuetype::BUYTRIES);      

      //Add (price - rev.share) to current level's pot
      addPot(_player.account, price);

      //Reset retries count (3 left)
      player.resetRetriesCount();
    }

    void PlayerLevel::cutRevenueShare(asset& revenue, const Const::revenuetype& revtype) {
      auto _meta = meta.getMeta();
      //shared amount proportinal to the payment itself - so same methods applied:
      asset revenueShare = (revtype == Const::revenuetype::UNJAIL 
        ? meta.unjailRevShare(revenue)
        : meta.buytryRevShare(revenue)
      );
      
      Channel::Channel channel(_self, player.getChannel());
      Branch::Branch branch(_self, getLevel().idbranch);

      //calculate sales channel share
      auto channelShare = (revenueShare * (channel.getRate() + meta.getMeta().slsrate)) / 100;
      auto branchShare = revenueShare - channelShare;

      //put sales channel fee into sales channel balance (!defer)
      channel.addBalance(channelShare, _self);
            
      //put revenue share into branch stakeholders' revenue (!defer, recursion to parent branches)
      branch.deferRevenueShare(branchShare);//branch winner will get some here, all winners along branch hierarchy

      revenue -= revenueShare;

      print(
        "Revenue share total: ", asset{revenueShare}, 
        "\nChannel share:", asset{channelShare}, 
        "\nBranch share: ", asset{branchShare}, 
        "\nLevel's pot gets: ", asset{revenue}, 
        "\n");
    }

    #pragma endregion
  }
}