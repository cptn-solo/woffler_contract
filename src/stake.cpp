#include <stake.hpp>

namespace Woffler {
  namespace Stake {    
    Stake::Stake(name self, uint64_t idstake) : 
      Entity<stakes, DAO, uint64_t>(self, idstake) {}

    DAO::DAO(stakes& _stakes, uint64_t idstake): 
      Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, idstake) {}
    
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
  }
}
