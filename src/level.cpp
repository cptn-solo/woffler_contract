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

    uint64_t Level::createLevel(const name& payer, const uint64_t& idbranch, const uint64_t& idparent, const uint64_t& generation, const uint64_t& idmeta, const bool& root) {
      meta.fetchByKey(idmeta);
      _entKey = nextPK();
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idparent = idparent;
        l.generation = generation;
        l.idmeta = idmeta;
        l.root = root;
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
      auto _player = player.getPlayer();

      /* Restrictions check */
      if (_level.idparent > 0) { //next/split levels
        if (_player.idlevel == _level.idparent)  { //standing in front of the level
          //Retries count to unlock levels in front of the player is restricted. 
          //Additional tries are bought by calling `buytries` action
          player.checkLevelUnlockTrialAllowed();
          player.useTry();
        } else if (_level.root) { //Unlock split branch from list - no retry checks implied
          Stake::Stake stake(_self, 0);
          stake.checkIsStakeholder(account, _level.idbranch);
        } else {
          check(false, "You can not unlock this level at the moment");
        }
      } else {
        //Root levels of root branches can be unlocked only by stakeholders, no retries restriction
        Stake::Stake stake(_self, 0);
        stake.checkIsStakeholder(account, _level.idbranch);
      }

      /* Generate cells */
      //getting branch meta to decide on level presets
      if (unlockTrial(account)) {
        switch (_player.status) {
          case Const::playerstate::NEXT: {
            /* set winner */
            Branch::Branch branch(_self, _level.idbranch);
            branch.setWinner(account);
            player.resetPositionAtLevel(_level.id);
            break;
          }
          case Const::playerstate::SPLIT: {
            /* change: splits now unlocked only by stakeholders */
            player.resetPositionAtLevel(_level.id);
            break;
          }
          default: {
            break;
          }
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
      auto _curl = getLevel();
      auto maxlvlgen = meta.getMeta().maxlvlgen;
      if (maxlvlgen > 0) {
        check(
          maxlvlgen > _curl.generation,
          "Current branch can't be extended any more due to its presets."
        );      
      }

      player.checkState(Const::playerstate::GREEN);
      
      auto _player = player.getPlayer();

      auto nextidx = getIndex<"byparent"_n>();
      auto nextlitr = nextidx.find(_curl.id);
      
      while (nextlitr != nextidx.end() && nextlitr->idbranch != _curl.idbranch)
        nextlitr++;

      if (nextlitr == nextidx.end()) { //create locked        
        Level nextL(_self);
        const uint64_t nextgen = _curl.generation + 1;
        const uint64_t nextId = nextL.createLevel(_player.account, _curl.idbranch, _curl.id, nextgen, _curl.idmeta, false);
        
        Branch::Branch branch(_self, _curl.idbranch);
        branch.updateTreeDept(_player.account, nextId, nextgen);

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
        Branch::Branch currentBranch(_self, _curl.idbranch);
        const asset splitPot = meta.splitPot(currentBranch.getBranch().potbalance, _curl.generation);
        currentBranch.subPot(_player.account, splitPot);

        //Create child branch and a locked level with "Red" cells, branch generation++
        //Move SPLIT_RATE% of solved pot to locked pot
        Branch::Branch childBranch(_self, 0);
        const uint64_t idchbranch = childBranch.createChildBranch(_player.account, _curl.idbranch, _curl.id, splitPot);

        update(_player.account, [&](auto& l) {
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
    
    void PlayerLevel::takeReward() {
      player.checkState(Const::playerstate::GREEN);

      auto _player = player.getPlayer();
      auto _curl = getLevel();
      
      //Cut TAKE_RATE% of solved pot and append to winner's vesting balance
      Branch::Branch branch(_self, _curl.idbranch);
      auto reward = meta.takeAmount(branch.getBranch().potbalance, _curl.generation);
      branch.subPot(_player.account, reward);// will close current branch if reward pot is drained

      //Set winner level result to TAKE and update result timestamp
      player.commitTake(reward, Utils::now() + meta.getMeta().tkintrvl);//retries reset, reward added to vesting
    }

    void PlayerLevel::unjailPlayer() {
      player.checkState(Const::playerstate::RED);

      auto _player = player.getPlayer();
      auto _curl = getLevel();

      //calculate unjail price
      Branch::Branch branch(_self, _curl.idbranch);
      auto price = meta.unjailPrice(branch.getBranch().potbalance, _curl.generation);

      //cut player's active balance with unjail payment value
      player.subBalance(price, _player.account);//will fail if balance not cover amount being cut

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(price, Const::revenuetype::UNJAIL);      

      //put remaining unjail payment into reward pot of the branch
      branch.addPot(_player.account, price);

      //reset player's state, retries and position in current level's zero cell
      player.resetPositionAtLevel(_curl.id);
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
      Branch::Branch branch(_self, _curl.idbranch);
      auto price = meta.buytryPrice(branch.getBranch().potbalance, _curl.generation);
      player.subBalance(price, _player.account);

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(price, Const::revenuetype::BUYTRIES);      

      //put remaining payment value into reward pot of the branch
      branch.addPot(_player.account, price);

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