#include <level.hpp>
#include <stake.hpp>
#include <branch.hpp>
#include <channel.hpp>

namespace Woffler {
  namespace Level {
    uint64_t Level::createLevel(const name& payer, const uint64_t& idbranch, const uint64_t& idparent, const uint64_t& generation, const uint64_t& idmeta, const bool& root, const asset& pot) {
      meta.fetchByKey(idmeta);
      branch.fetchByKey(idbranch);
      
      _entKey = nextPK();
      create(payer, [&](auto& l) {
        l.id = _entKey;
        l.idbranch = idbranch;
        l.idparent = idparent;
        l.generation = generation;
        l.idmeta = idmeta;
        l.root = root;
        l.potbalance = pot;
      });

      generateRedCells(payer);

      return _entKey;
    }

    void Level::addPot(const name& payer, const asset& pot) {
      update(payer, [&](auto& l) {
        l.potbalance += pot;
      });
    }
    
    void Level::unlockLevel(const name& account) {
      checkLockedLevel();

      Player::Player player(_self, account);
      auto _player = player.getPlayer();

      /* Restrictions check */
      if (_entity.idparent > 0) { //next/split levels
        if (_player.idlevel == _entity.idparent)  { //standing in front of the level
          //Retries count to unlock levels in front of the player is restricted. 
          //Additional tries are bought by calling `buytries` action
          player.checkLevelUnlockTrialAllowed();
          player.useTry();
        } else if (_entity.root) { //Unlock split branch from list - no retry checks implied
          Stake::Stake stake(_self);
          stake.checkIsStakeholder(account, _entity.idbranch);
        } else {
          check(false, "You can not unlock this level at the moment");
        }
      } else {
        //Root levels of root branches can be unlocked only by stakeholders, no retries restriction
        Stake::Stake stake(_self);
        stake.checkIsStakeholder(account, _entity.idbranch);
      }

      /* Generate cells */
      //getting branch meta to decide on level presets
      if (unlockTrial(account)) {
        switch (_player.status) {
          case Const::playerstate::NEXT: {
            /* set winner */
            branch.setWinner(account);
            player.resetPositionAtLevel(_entity.id);
            break;
          }
          case Const::playerstate::SPLIT: {
            /* change: splits now unlocked only by stakeholders */
            player.resetPositionAtLevel(_entity.id);
            break;
          }
          default: {
            break;
          }
        }
      }
    }

    void Level::generateRedCells(const name& payer) {
      auto rnd = randomizer::getInstance(payer, _entKey);
      auto redcnt = meta.getMeta().lvlreds;
      update(payer, [&](auto& l) {
        l.redcells = generateCells(rnd, redcnt);
      });
    }

    bool Level::unlockTrial(const name& payer) {
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

    Const::playerstate Level::cellTypeAtPosition(const uint8_t& position) {
      check(position <= Const::lvlLength, "Position in the level can't exceed 16");

      auto status = Const::playerstate::SAFE;
      uint16_t pos16 = 1<<position;
      if (Utils::hasIntersection(pos16, _entity.redcells)) {
        status = Const::playerstate::RED;
      } else if (Utils::hasIntersection(pos16, _entity.greencells)) {
        status = Const::playerstate::GREEN;
      }
      return status;
    }

    void Level::regenCells(const name& owner) {
      generateRedCells(owner);
      unlockTrial(owner);
    }

    void Level::rmLevel() {
      remove();
    }

    void Level::checkLockedLevel() {
      check(
        _entity.locked,
        "Level is already unlocked."
      );
    }

    void Level::checkUnlockedLevel() {
      check(
        !_entity.locked,
        "Level is locked."
      );
    }

    #pragma region ** PlayerLevel **

    void PlayerLevel::tryTurn() {
      player.checkState(Const::playerstate::SAFE);
      
      /* Turn logic */
      //find player's current level 
      checkUnlockedLevel();//just to read level's data, not nesessary to check for lock - no way get to locked level
      branch.checkNotClosed();
      auto triesleft = _player.triesleft;
      auto tryposition = _player.tryposition;
      if (triesleft > 0) {
        //get current position and produce tryposition by generating random offset
        auto rnd = randomizer::getInstance(_player.account, _entKey);
        tryposition = (_player.currentposition + rnd.range(Const::tryturnMaxDistance)) % Const::lvlLength;
        triesleft = player.useTry(tryposition);    
      }
      if (triesleft == 0) {
        auto status = cellTypeAtPosition(tryposition);
        player.commitTurn(status);
      }
    }

    void PlayerLevel::commitTurn() {
      player.checkState(Const::playerstate::SAFE);
      branch.checkNotClosed();

      auto status = cellTypeAtPosition(_player.tryposition);
      player.commitTurn(status);
    }    

    void PlayerLevel::cancelTake() {
      player.checkState(Const::playerstate::TAKE);
      branch.checkNotClosed();//must be safe to claim takes on closed branches

      branch.addPot(_player.account, _player.vestingbalance);
      addPot(_player.account, _player.vestingbalance);

      player.clearVesting();
    }

    void PlayerLevel::claimSafe() {
      branch.checkNotClosed();      
      check(
        _player.status == Const::playerstate::GREEN ||
        _player.status == Const::playerstate::NEXT ||
        _player.status == Const::playerstate::SPLIT,
        "Only player in GREEN/NEXT/SPLIT state can apply for repositon to 0 cell (safe)."
      );

      player.resetPositionAtLevel(_player.idlevel);
    }

    void PlayerLevel::claimRed() {
      branch.checkNotClosed();      
      player.checkState(Const::playerstate::RED);
      check(
        _entity.idparent != 0 || !_meta.startjailed,
        "You can only call `unjail` or `switchbrnch` from your current state"
      );
      uint64_t idlevel = (_entity.idparent != 0 ? _entity.idparent : _entity.id);
      player.resetPositionAtLevel(idlevel);
    }

    void PlayerLevel::claimTake() {      
      player.checkState(Const::playerstate::TAKE);
      if (_branch.closed == 0) {//must be safe to claim takes on closed branches
        if (_player.resulttimestamp > Utils::now()) {
          auto expiredAfter = _player.resulttimestamp - Utils::now();
          check(
            expiredAfter <= 0,
            string("TAKE state did not expired yet. Seconds left until expiration: ") + std::to_string(expiredAfter)
          );        
        }    
        player.resetPositionAtLevel(_player.idlevel);
      } else {
        player.switchRootLevel(0, Const::playerstate::INIT);
      }
      //Move player's vested balance to active balance
      player.claimVesting();
    }

    void PlayerLevel::nextLevel() {      
      branch.checkNotClosed();
      
      auto maxlvlgen = _meta.maxlvlgen;
      if (maxlvlgen > 0) {
        check(
          maxlvlgen > _entity.generation,
          "Current branch can't be extended any more due to its presets."
        );      
      }

      player.checkState(Const::playerstate::GREEN);
      
      auto nextidx = getIndex<"byparent"_n>();
      auto nextlitr = nextidx.find(_entity.id);
      
      while (nextlitr != nextidx.end() && nextlitr->idbranch != _entity.idbranch)
        nextlitr++;

      if (nextlitr == nextidx.end()) { //create locked      

        Level nextL(_self);

        const asset nextpot = meta.nextPot(_entity.potbalance);
        const uint64_t nextgen = _entity.generation + 1;
        const uint64_t nextId = nextL.createLevel(_player.account, _entity.idbranch, _entity.id, nextgen, _entity.idmeta, false, nextpot);
        
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
      player.checkState(Const::playerstate::GREEN);
      branch.checkNotClosed();

      auto splitidx = getIndex<"byparent"_n>();
      auto splitlitr = splitidx.find(_entity.id);
      
      while (splitlitr != splitidx.end() && splitlitr->idbranch == _entity.idbranch)
        splitlitr++;

      //Child branch exists for winner's current level?
      if (splitlitr == splitidx.end()) {

        const asset splitPot = meta.splitPot(_entity.potbalance);

        branch.subPot(_player.account, splitPot);

        //Create child branch and a locked level with "Red" cells, branch generation++
        //Move SPLIT_RATE% of solved pot to locked pot
        Branch::Branch chbranch(_self, 0);
        const uint64_t idchbranch = chbranch.createChildBranch(_player.account, _entity.idbranch, _entity.id, splitPot);

        update(_player.account, [&](auto& l) {
          l.idchbranch = idchbranch;
          l.potbalance -= splitPot;
        });
        player.commitTurn(Const::playerstate::SPLIT);
      } 
      else {
        Branch::Branch chbranch(_self, splitlitr->idbranch);
        chbranch.checkNotClosed();

        if (splitlitr->locked) {
          player.commitTurn(Const::playerstate::SPLIT);
        } 
        else {
          check(!splitlitr->locked, "Split level locked. Please unlock it first using 'unlocklvl' action.");
          player.resetPositionAtLevel(splitlitr->id);
        }
      }
    }
    
    void PlayerLevel::takeReward() {
      player.checkState(Const::playerstate::GREEN);      
      branch.checkNotClosed();
      
      //Cut TAKE_RATE% of solved pot and append to winner's vesting balance
      const asset reward = meta.takeAmount(_entity.potbalance, _entity.generation, _branch.winlevgen);

      update(_player.account, [&](auto& l) {
        l.potbalance -= reward;
      });

      branch.subPot(_player.account, reward);// will close current branch if reward pot is drained

      //Set winner level result to TAKE and update result timestamp
      player.commitTake(reward, Utils::now() + meta.getMeta().tkintrvl);//retries reset, reward added to vesting
    }

    void PlayerLevel::unjailPlayer() {
      player.checkState(Const::playerstate::RED);
      branch.checkNotClosed();//the only option is to end the game by switchbranch

      //calculate unjail price
      auto price = meta.unjailPrice(_entity.potbalance, _entity.generation);

      //cut player's active balance with unjail payment value
      player.subBalance(price, _player.account);//will fail if balance not cover amount being cut

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(price, Const::revenuetype::UNJAIL);      

      //put remaining unjail payment into reward pot of the branch
      branch.addPot(_player.account, price);

      update(_player.account, [&](auto& l) {
        l.potbalance += price;
      });

      //reset player's state, retries and position in current level's zero cell
      player.resetPositionAtLevel(_entity.id);
    }

    void PlayerLevel::buyRetries() {
      check(_player.triesleft == 0, "Retries count must be 0.");
      check(
        _player.status == Const::playerstate::NEXT ||
        _player.status == Const::playerstate::SPLIT,
        "Extra retries are available only for next/split level unlock action."
      );

      if (_player.status == Const::playerstate::NEXT) {
        branch.checkNotClosed();
      }
      else if (_player.status == Const::playerstate::NEXT) {
        Branch::Branch splitbranch(_self, _entity.idchbranch);
        splitbranch.checkNotClosed();
      }

      //Player's balance covers price? 
      //Cut player's balance with price 
      auto price = meta.buytryPrice(_entity.potbalance, _entity.generation);
      player.subBalance(price, _player.account);

      //share revenue with branch, branch winner, branch hierarchy and referrer
      cutRevenueShare(price, Const::revenuetype::BUYTRIES);      

      //put remaining payment value into reward pot of the branch
      branch.addPot(_player.account, price);

      update(_player.account, [&](auto& l) {
        l.potbalance += price;
      });

      //Reset retries count (3 left)
      player.resetRetriesCount();
    }

    void PlayerLevel::cutRevenueShare(asset& revenue, const Const::revenuetype& revtype) {
      //shared amount proportinal to the payment itself - so same methods applied:
      asset revenueShare = (revtype == Const::revenuetype::UNJAIL 
        ? meta.unjailRevShare(revenue)
        : meta.buytryRevShare(revenue)
      );
      
      Channel::Channel channel(_self, player.getChannel());

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