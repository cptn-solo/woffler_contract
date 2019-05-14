#include <stake.hpp>

namespace Woffler {
  namespace Stake {    
    Stake::Stake(name self, uint64_t idstake) : 
      Entity<stakes, DAO, uint64_t>(self, idstake) {}

    DAO::DAO(stakes& _stakes, uint64_t idstake): 
      Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, idstake) {}
    
    DAO::DAO(stakes& _stakes, stakes::const_iterator itr): 
      Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, itr) {}
    
    void Stake::addStake(name owner, uint64_t idbranch, asset amount) {
      Branch::Branch branch(_self, idbranch);
      branch.checkBranch();

      //cut owner's active balance for pot value (will fail if not enough funds)
      Player::Player player(_self, owner);
      player.subBalance(amount, owner);
      
      auto _branch = branch.getBranch();

      if (_branch.generation > 1) {
        //non-root branches don't directly share profit with contract's account (house)
        registerStake(owner, idbranch, amount);
      } 
      else {
        //register players's and house stake
        auto houseStake = (amount * Const::houseShare) / 100;
        auto playerStake = (amount - houseStake);

        registerStake(owner, idbranch, playerStake);
        registerStake(_self, idbranch, houseStake);
      }

      //if root level is created already - append staked value to the root level's pot
      if(_branch.idrootlvl > 0) {
        Level::Level level(_self, _branch.idrootlvl);
        level.checkLevel();
        level.addPot(owner, amount);
      }      
    }

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

    void Stake::deferRevenueShare(uint64_t idbranch, asset amount, asset totalstake) {
      transaction out{};
      out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "tipstkhldrs"_n, std::make_tuple(idbranch, amount, totalstake));
      out.delay_sec = 1;
      out.send(Utils::now(), _self);
    }

    void Stake::allocateRevshare(uint64_t idbranch, asset amount, asset totalstake) {
      auto stkidx = getIndex<"bybranch"_n>();
      auto stkitr = stkidx.lower_bound(idbranch);
      while(stkitr != stkidx.end()) {
        auto share = (amount * stkitr->stake.amount) / totalstake.amount;
        _idx.modify(*stkitr, _self, [&](auto& s) {
          s.revenue += share;     
        });
        stkitr++;
      }
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
