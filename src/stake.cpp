#include <stake.hpp>
#include <branch.hpp>

namespace Woffler {
  namespace Stake {    
    Stake::Stake(name self, uint64_t idstake) : 
      Entity<stakes, DAO, uint64_t>(self, idstake) {}

    DAO::DAO(stakes& _stakes, uint64_t idstake): 
      Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, idstake) {}
    
    DAO::DAO(stakes& _stakes, stakes::const_iterator itr): 
      Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, itr) {}
    
    void Stake::registerStake(name owner, uint64_t idbranch, asset amount) {
      //find stake and add amount, or emplace if not found
      auto ownedBranchId = Utils::combineIds(owner.value, idbranch);    
      auto stkidx = getIndex<"byownedbrnch"_n>();
      const auto& stake = stkidx.find(ownedBranchId);          

      if (stake == stkidx.end()) {
        uint64_t nextId = nextPK();
        create(owner, [&](auto& s) {
          s.id = nextId;
          s.idbranch = idbranch;
          s.owner = owner;
          s.stake = amount;
        });
      } 
      else {
        //actually we can modify item found by any index: https://github.com/EOSIO/eos/issues/5335
        //in this case we've found an item to be modified already, stake is the pointer to it:
        _idx.modify(*stake, owner, [&](auto& s) {
          s.stake += amount;     
        });    
      }
    }

    void Stake::branchStake(name owner, uint64_t idbranch, asset& total, asset& owned) {
      //calculating branch stake total (all stakeholders)
      auto stkidx = getIndex<"bybranch"_n>();
      auto stkitr = stkidx.lower_bound(idbranch);
      while(stkitr != stkidx.end()) {
        total += stkitr->stake;
        if (stkitr->owner == owner) 
          owned += stkitr->stake;

        stkitr++;
      }
    }

    asset Stake::branchStake(uint64_t idbranch) {
      //calculating branch stake total (all stakeholders)
      auto stkidx = getIndex<"bybranch"_n>();
      auto stkitr = stkidx.lower_bound(idbranch);
      auto total = asset{0, Const::acceptedSymbol};
      while(stkitr != stkidx.end()) {
        total += stkitr->stake;
        stkitr++;
      }
      return total;
    }

    void Stake::allocateRevshare(name owner, uint64_t idbranch, uint64_t txid) {
      auto stkidx = getIndex<"byownedbrnch"_n>();
      auto stkitr = stkidx.find(Utils::combineIds(owner.value, idbranch));
      check(stkitr != stkidx.end(), "No stake in branch for this owner");

      Branch::Branch branch(_self, idbranch);
      auto _branch = branch.getBranch();

      tipstakes _tipstakes(_self, _self.value);
      auto tipitr = _tipstakes.find(txid);
      check(tipitr != _tipstakes.end(), "No tip found");

      //start with 1st tip in the queue or will miss all tips before one claimed first (no backward movement possible)
      check(stkitr->revtxid == 0 || tipitr->id > stkitr->revtxid, "Earlier tips must be allocated first. ");

      auto share = (tipitr->amount * stkitr->stake.amount) / _branch.totalstake;
      if (share >= tipitr->amount) {//last claimer removes tip record from table to preserve RAM
        share = tipitr->amount;
        tipstakes.erase(*tipitr);
      } else {
        _tipstakes.modify(*tipitr, _self, [&](auto& t) {
          t.amount -= share;     
        });
      }
      
      _idx.modify(*stkitr, _self, [&](auto& s) {
        s.revenue += share;     
        s.revtxid = txid;
      });
    }

    void Stake::rmStake() {
      remove();
    }

    void Stake::checkIsStakeholder(name owner, uint64_t idbranch) {
      auto ownedBranchId = Utils::combineIds(owner.value, idbranch);    
      auto stkidx = getIndex<"byownedbrnch"_n>();
      const auto& stake = stkidx.find(ownedBranchId);
      check(
        stake != stkidx.end(),
        "Account doesn't own stake in the branch."
      );

    }
  }
}
