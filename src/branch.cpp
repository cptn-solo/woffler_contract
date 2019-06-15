#include <branch.hpp>
#include <player.hpp>

namespace Woffler {
  namespace Branch {
    //create root branch with root level after meta is created/selected from existing
    void Branch::createBranch(const name& owner, const uint64_t& idmeta, const asset& pot) {
      BranchMeta::BranchMeta meta(_self, idmeta);
      auto _meta = meta.getMeta();

      auto minPot = (_meta.stkmin * 100) / _meta.spltrate;
      check(
        minPot <= pot,
        string("Branch minimum pot is ")+minPot.to_string().c_str()
      );

      //cut owner's active balance for pot value (will fail if not enough funds)
      Player::Player player(_self, owner);
      player.subBalance(pot, owner);

      //create branch record
      _entKey = nextPK();
      create(owner, [&](auto& b) {
        b.id = _entKey;
        b.idmeta = idmeta;
        b.potbalance = pot;
      });

      //register players's and house stake
      auto houseStake = (pot * Const::houseShare) / 100;
      auto playerStake = (pot - houseStake);

      appendStake(owner, playerStake);
      appendStake(_self, houseStake);

      Level::Level level(_self);
      uint64_t idlevel = level.createLevel(owner, _entKey, 0, 1, idmeta, true, pot);

      setRootLevel(owner, idlevel, 1);
    }

    uint64_t Branch::createChildBranch(const name& owner, const uint64_t& pidbranch, const uint64_t& pidlevel, const asset& pot) {
      _entKey = nextPK();
      auto parent = _idx.find(pidbranch);
      check(parent != _idx.end(), "Parent branch not found");
      create(owner, [&](auto& b) {
        b.id = _entKey;
        b.idmeta = parent->idmeta;
        b.idparent = pidbranch;
        b.generation = (parent->generation + 1);
        b.potbalance = pot;
      });
      
      //here is the change: child branch creator got stake. all next green can't unlock this branch until `stkaddval`
      appendStake(owner, pot);

      Level::Level level(_self);
      uint64_t idlevel = level.createLevel(owner, _entKey, pidlevel, 1, parent->idmeta, true, pot);

      setRootLevel(owner, idlevel, 1);        

      return _entKey;
    }

    void Branch::addStake(const name& owner, const asset& amount) {
      BranchMeta::BranchMeta meta(_self, _entity.idmeta);
      auto _meta = meta.getMeta();

      auto threshold = meta.stakeThreshold(_entity.potbalance);

      check(amount >= threshold, string("Amount must be >= ")+threshold.to_string().c_str());

      //cut owner's active balance for pot value (will fail if not enough funds)
      Player::Player player(_self, owner);
      player.subBalance(amount, owner);

      if (_entity.generation > 1) {
        //non-root branches don't directly share profit with contract's account (house)
        appendStake(owner,amount);
      }
      else {
        //register players's and house stake
        auto houseStake = (amount * Const::houseShare) / 100;
        auto playerStake = (amount - houseStake);

        appendStake(owner, playerStake);
        appendStake(_self, houseStake);
      }

      addPot(owner, amount);

      Level::Level rootlevel(_self, _entity.idrootlvl);
      rootlevel.addPot(owner, amount);
    }

    void Branch::addPot(const name& payer, const asset& pot) {
      update(payer, [&](auto& b) {
        b.potbalance += pot;
      });
    }
    
    void Branch::subPot(const name& payer, const asset& take) {
      print("Branch::subPot ", _entity.potbalance, take);
      check(_entity.potbalance >= take, "Branch pot balanse must cover reward amount");
      update(payer, [&](auto& b) {
        b.potbalance -= take;
      });
      
      if (_entity.potbalance.amount == 0)
        closeBranch();      
    }

    void Branch::closeBranch() {
      update(_self, [&](auto& b) {
        b.closed = Utils::now();
      });
      
      auto idparent = _entity.idparent;
      if (idparent > 0) {
        Branch parent(_self, idparent);
        parent.update(_self,[&](auto& b) {
          b.openchildcnt--;
        });
      }
    }

    void Branch::appendStake(const name& owner, const asset& amount) {
      stake.registerStake(owner, _entKey, amount);

      update(_self, [&](auto& b) {
        b.totalstake += amount;
      });
    }

    void Branch::setRootLevel(const name& payer, const uint64_t& idrootlvl, const uint64_t& generation) {
      update(payer, [&](auto& b) {
        b.idrootlvl = idrootlvl;
        b.winlevel = idrootlvl;
        b.winlevgen = generation;
      });
    }

    void Branch::updateTreeDept(const name& payer, const uint64_t& idlevel, const uint64_t& generation) {
      update(payer, [&](auto& b) {
        b.winlevel = idlevel;
        b.winlevgen = generation;
        b.openchildcnt++;
      });
    }

    void Branch::setWinner(const name& player) {
      update(player, [&](auto& b) {
        b.winner = player;
      });
    }

    void Branch::deferRevenueShare(const asset& amount) {
      update(_self, [&](auto& b) {
        b.totalrvnue += amount;
        b.tipprocessed = 0;
      });
    }

    void Branch::deferRevenueShare(const asset& amount, const uint64_t& idbranch) {
      Branch branch(_self, idbranch);
      branch.deferRevenueShare(amount);
    }

    //this method always run in context of a deferred transaction and only for unprocessed branches
    void Branch::allocateRevshare() {
      check(_entity.tipprocessed == 0, "Branch already processed");
      /*
        Implementation notice.
        To avoid need to generate detailed "billing" record for each revenue event, generated by levels, current implementation
        relies on incremental approach: each time branch is tipped by some revenue event (unjail, etc.), its total revenue field
        appended and tipprocessed flag set to false. This current allocateRevshare method does the processing of revenue event
        and resets the flag back to true upon processing is complete.
        Allocation itself relies on values of already processed (allocated) revenue stored in the branch data.
        Claim process in its turn relies on values of already claimed revenue stored in the stakeholders ledger (see Stake namespace).
        Some minor drawbacks of the implementation:
        - if revenue events will occur often, branch state will stay "unprocessed" most of the time.
        - to "process" the branch after revenue event, one should call `revshare` action to create deferred processing transaction for
        unprocessed branches.
        Despite of these drawbacks, stakeholders can claim their revenue once per day/week/month, while `revshare` action can be called
        by any account even not registered in the game contract as a player.
      */

      auto totalrvnue = _entity.totalrvnue;//for unprocessed branch this amount is "gross" - contains parent and winner shares
      auto parentrvnue = _entity.parentrvnue;//"old" value, doesn't yet include new tip until "processed"
      auto winnerrvnue = _entity.winnerrvnue;//"old" value, doesn't yet include new tip until "processed"

      //get parent branch
      if (_entity.idparent > 0) {
        //if exists then cut (amount/generation) and call deferRevenueShare on parent branch

        parentrvnue = totalrvnue / _entity.generation;//totalrvnue is now bigger then it was when parentrvnue was last calculated, so this is an "increment"
        totalrvnue -= parentrvnue;
        auto delta = parentrvnue - _entity.parentrvnue;
        deferRevenueShare(delta, _entity.idparent);//only delta revenue amount should be paid at a time
        print("Parent branch <", std::to_string(_entity.idparent), "> get: ", asset{delta}, ".\n");
      }
      //cut branch winner amount
      if (_entity.winner) {
        BranchMeta::BranchMeta meta(_self, _entity.idmeta);
        auto _meta = meta.getMeta();
        winnerrvnue = (totalrvnue * _meta.winnerrate) / 100;

        totalrvnue -= winnerrvnue;

        Player::Player player(_self, _entity.winner);
        auto delta = winnerrvnue - _entity.winnerrvnue;
        player.addBalance(delta, _self);//only delta revenue amount should be paid at a time
        print("Branch winner <", name(_entity.winner), "> get: ", asset{delta}, ".\n");
      }

      update(_self, [&](auto& b) {
        b.totalrvnue = totalrvnue;
        b.parentrvnue = parentrvnue;
        b.winnerrvnue = winnerrvnue;
        b.tipprocessed = Utils::now();
      });
    }

    void Branch::rmBranch() {
      remove();
    }

    void Branch::checkBranch() {
      check(
        isEnt(),
        "No branch found for id"
      );
    }

    void Branch::checkStartBranch() {
      check(
        _entity.generation == 1,
        "Player can start only from root branch"
      );
      check(
        _entity.idrootlvl != 0,
        "Branch has no root level yet."
      );
    }

    void Branch::checkNotClosed() {
      check(
        _entity.closed == 0,
        "Branch is closed and cant be played any more"
      );
    }

    void Branch::checkEmptyBranch() {
      check(
        _entity.idrootlvl == 0,
        "Root level already exists"
      );
    }

    void Branch::checkBranchMetaNotUsed(const uint64_t& idmeta) {
      check(
        !isIndexedByMeta(idmeta),
        "Branch metadata is already used in branches."
      );
    }

    bool Branch::isIndexedByMeta(const uint64_t& idmeta) {
      auto idxbymeta = getIndex<"bymeta"_n>();
      auto itrbymeta = idxbymeta.find(idmeta);
      return itrbymeta != idxbymeta.end();
    }
  }
}