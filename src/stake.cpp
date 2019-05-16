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

    void Stake::claimRevenue(name owner, uint64_t idbranch) {
      auto stkidx = getIndex<"byownedbrnch"_n>();
      auto stkitr = stkidx.find(Utils::combineIds(owner.value, idbranch));
      check(stkitr != stkidx.end(), "No stake in branch for this owner");

      Branch::Branch branch(_self, idbranch);
      auto _branch = branch.getBranch();
      check(_branch.tipprocessed, "Branch revenue is not yet available for claiming after last tip. Please run revshare action first");

      auto share = ((_branch.totalrvnue - stkitr->revenue) * stkitr->stake.amount) / _branch.totalstake.amount;
      _idx.modify(*stkitr, owner, [&](auto& s) {
        s.revenue += share;
      });

      Player::Player player(_self, owner);
      player.addBalance(share, owner);
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
