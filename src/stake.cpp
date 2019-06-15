#include <stake.hpp>
#include <branch.hpp>

namespace Woffler {
  namespace Stake {    
    void Stake::registerStake(const name& owner, const uint64_t& idbranch, const asset& amount) {
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

    void Stake::branchStake(const name& owner, const uint64_t& idbranch, asset& total, asset& owned) {
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

    void Stake::claimRevenue(const name& owner, const uint64_t& idbranch) {
      auto stkidx = getIndex<"byownedbrnch"_n>();
      auto stkitr = stkidx.find(Utils::combineIds(owner.value, idbranch));
      check(stkitr != stkidx.end(), "No stake in branch for this owner");

      Branch::Branch branch(_self, idbranch);
      auto _branch = branch.getBranch();
      check(_branch.tipprocessed != 0, "Branch revenue is not yet available for claiming after last tip. Please run <revshare> action first");

      auto share = (_branch.totalrvnue * stkitr->stake.amount) / _branch.totalstake.amount;
      auto addition = share - stkitr->revenue;
      if (addition.amount > 0) {
        _idx.modify(*stkitr, owner, [&](auto& s) {
          s.revenue += addition;
        });
        Player::Player player(_self, owner);
        player.addBalance(addition, owner);
        print("Claimed new revenue: ", asset{addition}, " , current claimed revenue: ", asset{stkitr->revenue}, " \n");
      } 
      else {
        print("No new revenue, current share: ", asset{share}, " , current claimed revenue: ", asset{stkitr->revenue}, " \n");
      }
    }

    void Stake::rmStake() {
      remove();
    }

    void Stake::checkIsStakeholder(const name& owner, const uint64_t& idbranch) {
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
