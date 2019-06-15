#include <branch.hpp>
#include <player.hpp>

namespace Woffler {
  namespace Branch {
    //create root branch with root level after meta is created/selected from existing
    void Branch::createBranch(name owner, uint64_t idmeta, asset pot) {
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
      _branch = create(owner, [&](auto& b) {
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
      _branch = create(owner, [&](auto& b) {
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

    void Branch::addStake(name owner, asset amount) {
      BranchMeta::BranchMeta meta(_self, _branch.idmeta);
      auto _meta = meta.getMeta();

      auto threshold = meta.stakeThreshold(_branch.potbalance);

      check(amount >= threshold, string("Amount must be >= ")+threshold.to_string().c_str());

      //cut owner's active balance for pot value (will fail if not enough funds)
      Player::Player player(_self, owner);
      player.subBalance(amount, owner);

      if (_branch.generation > 1) {
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

      Level::Level rootlevel(_self, _branch.idrootlvl);
      rootlevel.addPot(owner, amount);
    }

    void Branch::addPot(name payer, asset pot) {
      _branch = update(payer, [&](auto& b) {
        b.potbalance += pot;
      });
    }
    
    void Branch::subPot(name payer, asset take) {
      check(getBranch().potbalance >= take, "Branch pot balanse must cover reward amount");
      _branch = update(payer, [&](auto& b) {
        b.potbalance -= take;
      });
      
      if (getBranch().potbalance.amount == 0)
        closeBranch();      
    }

    void Branch::closeBranch() {
      _branch = update(_self, [&](auto& b) {
        b.closed = Utils::now();
      });
      
      auto idparent = _branch.idparent;
      if (idparent > 0) {
        Branch parent(_self, idparent);
        parent.update(_self,[&](auto& b) {
          b.openchildcnt--;
        });
      }
    }

    void Branch::appendStake(name owner, asset amount) {
      stake.registerStake(owner, _entKey, amount);

      _branch = update(_self, [&](auto& b) {
        b.totalstake += amount;
      });
    }

    void Branch::setRootLevel(name payer, uint64_t idrootlvl, uint64_t generation) {
      _branch = update(payer, [&](auto& b) {
        b.idrootlvl = idrootlvl;
        b.winlevel = idrootlvl;
        b.winlevgen = generation;
      });
    }

    void Branch::updateTreeDept(name payer, uint64_t idlevel, uint64_t generation) {
      _branch = update(payer, [&](auto& b) {
        b.winlevel = idlevel;
        b.winlevgen = generation;
        b.openchildcnt++;
      });
    }

    void Branch::setWinner(name player) {
      _branch = update(player, [&](auto& b) {
        b.winner = player;
      });
    }

    void Branch::deferRevenueShare(asset amount) {
      _branch = update(_self, [&](auto& b) {
        b.totalrvnue += amount;
        b.tipprocessed = 0;
      });
    }

    void Branch::deferRevenueShare(asset amount, uint64_t idbranch) {
      Branch branch(_self, idbranch);
      branch.deferRevenueShare(amount);
    }

    //this method always run in context of a deferred transaction and only for unprocessed branches
    void Branch::allocateRevshare() {
      check(_branch.tipprocessed == 0, "Branch already processed");
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

      auto totalrvnue = _branch.totalrvnue;//for unprocessed branch this amount is "gross" - contains parent and winner shares
      auto parentrvnue = _branch.parentrvnue;//"old" value, doesn't yet include new tip until "processed"
      auto winnerrvnue = _branch.winnerrvnue;//"old" value, doesn't yet include new tip until "processed"

      //get parent branch
      if (_branch.idparent > 0) {
        //if exists then cut (amount/generation) and call deferRevenueShare on parent branch

        parentrvnue = totalrvnue / _branch.generation;//totalrvnue is now bigger then it was when parentrvnue was last calculated, so this is an "increment"
        totalrvnue -= parentrvnue;
        auto delta = parentrvnue - _branch.parentrvnue;
        deferRevenueShare(delta, _branch.idparent);//only delta revenue amount should be paid at a time
        print("Parent branch <", std::to_string(_branch.idparent), "> get: ", asset{delta}, ".\n");
      }
      //cut branch winner amount
      if (_branch.winner) {
        BranchMeta::BranchMeta meta(_self, _branch.idmeta);
        auto _meta = meta.getMeta();
        winnerrvnue = (totalrvnue * _meta.winnerrate) / 100;

        totalrvnue -= winnerrvnue;

        Player::Player player(_self, _branch.winner);
        auto delta = winnerrvnue - _branch.winnerrvnue;
        player.addBalance(delta, _self);//only delta revenue amount should be paid at a time
        print("Branch winner <", name(_branch.winner), "> get: ", asset{delta}, ".\n");
      }

      _branch = update(_self, [&](auto& b) {
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
        _branch.generation == 1,
        "Player can start only from root branch"
      );
      check(
        _branch.idrootlvl != 0,
        "Branch has no root level yet."
      );
    }

    void Branch::checkNotClosed() {
      check(
        _branch.closed == 0,
        "Branch is closed and cant be played any more"
      );
    }

    void Branch::checkEmptyBranch() {
      check(
        _branch.idrootlvl == 0,
        "Root level already exists"
      );
    }

    void Branch::checkBranchMetaNotUsed(uint64_t idmeta) {
      check(
        !isIndexedByMeta(idmeta),
        "Branch metadata is already used in branches."
      );
    }

    bool Branch::isIndexedByMeta(uint64_t idmeta) {
      auto idxbymeta = getIndex<"bymeta"_n>();
      auto itrbymeta = idxbymeta.find(idmeta);
      return itrbymeta != idxbymeta.end();
    }
  }
}