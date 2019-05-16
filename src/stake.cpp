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
    
    void Stake::registerStake(name owner, uint64_t idbranch, asset amount, uint64_t revtxid) {
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
          s.revtxid = revtxid;
        });
      } 
      else {
        //actually we can modify item found by any index: https://github.com/EOSIO/eos/issues/5335
        //in this case we've found an item to be modified already, stake is the pointer to it:
        _idx.modify(*stake, owner, [&](auto& s) {
          s.stake += amount;     
          s.revtxid = revtxid;
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

    void Stake::claimTip(name owner, uint64_t txid) {
      branchtips _branchtips(_self, _self.value);
      auto tipitr = _branchtips.find(txid);
      check(tipitr != _branchtips.end(), "No tip found");
      check(tipitr->processed, "Tip must be processed befor claim. Tip processing is a deferred action initiated by one of 'lightweight' actions: tryturn, committurn, claimred and some other");

      auto stkidx = getIndex<"byownedbrnch"_n>();
      auto stkitr = stkidx.find(Utils::combineIds(owner.value, tipitr->idbranch));
      check(stkitr != stkidx.end(), "No stake in branch for this owner");

      Branch::Branch branch(_self, tipitr->idbranch);
      auto _branch = branch.getBranch();

      //start with 1st tip in the queue or will miss all tips before one claimed first 
      //(no backward movement possible as it would require to implement additional relations between tips and stakeholders)
      check(stkitr->lasttipid == 0 || tipitr->id > stkitr->lasttipid, "Earlier tips must be allocated first. ");

      auto share = (tipitr->amount * stkitr->stake.amount.value) / tipitr->base.value;
      if (share >= tipitr->unclaimed) {//last claimer removes tip record from table to release RAM
        share = tipitr->unclaimed;
        branchtips.erase(*tipitr);
      } else {
        _branchtips.modify(*tipitr, _self, [&](auto& t) {
          t.unclaimed -= share;     
        });
      }

      _idx.modify(*stkitr, same_payer, [&](auto& s) {
        s.revenue += share;     
        s.lasttipid = txid;
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
