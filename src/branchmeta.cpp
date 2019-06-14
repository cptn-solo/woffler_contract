#include <branchmeta.hpp>
#include <branch.hpp>

namespace Woffler {
  namespace BranchMeta {
    BranchMeta::BranchMeta(name self, uint64_t idmeta) :
      Entity<brnchmetas, DAO, uint64_t>(self, idmeta) {
      }

    DAO::DAO(brnchmetas& _brnchmetas, uint64_t idmeta):
        Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>::Accessor(_brnchmetas, idmeta) {}

    DAO::DAO(brnchmetas& _brnchmetas, brnchmetas::const_iterator itr):
        Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>::Accessor(_brnchmetas, itr) {}

    wflbrnchmeta BranchMeta::getMeta() {
      return getEnt<wflbrnchmeta>();
    }

    asset BranchMeta::nextPot(const asset& pot) {
      auto _meta = getMeta();
      asset nxtPot = (pot * _meta.nxtrate) / 100;
      if (nxtPot < _meta.potmin)
        nxtPot = pot;
      return nxtPot;
    }

    asset BranchMeta::splitPot(const asset& pot) {
      auto _meta = getMeta();
      //solved * SPLIT_RATE% >= STAKE_MIN?      
      auto minPot = (_meta.stkmin * 100) / _meta.spltrate;
      check(
        pot >= minPot,
        string("Reward pot of the current level is too small, should be at least ") + minPot.to_string().c_str()
      );

      auto splitAmount = (pot * _meta.spltrate) / 100;
      return splitAmount;
    }

    asset BranchMeta::takeAmount(const asset& pot, const uint64_t& generation, const uint64_t& winlevgen) {
      auto _meta = getMeta();

      if (_meta.maxlvlgen > 0 && _meta.maxlvlgen == generation)//last level winner gets all remaining pot
        return pot;

      auto reward = (pot * _meta.tkrate) / 100;
      
      check(reward.amount > 0, "Reward amount must be > 0");

      if (_meta.takemult > 0) 
        reward *= (_meta.takemult * generation); 

      if (reward > pot)
        return pot;

      return reward;
    }

    asset BranchMeta::stakeThreshold(const asset& pot) {
      //stake amounts derived from total branch pot, not current level's amount
      auto _meta = getMeta();
      auto price = (pot * _meta.stkrate) / 100;
      if (price < _meta.stkmin)
        price = _meta.stkmin;

      return price;
    }

    asset BranchMeta::unjailPrice(const asset& pot, const uint64_t& generation) {
      auto _meta = getMeta();
      auto price = (pot * _meta.unjlrate) / 100;

      if (_meta.unljailmult > 0)
        price *= (_meta.unljailmult * generation);

      if (price < _meta.unjlmin)
        price = _meta.unjlmin;

      return price;
    }

    asset BranchMeta::unjailRevShare(const asset& revenue) {
      return (revenue * getMeta().unjlrate) / 100;
    }

    asset BranchMeta::buytryPrice(const asset& pot, const uint64_t& generation) {
      auto _meta = getMeta();
      auto price = (pot * _meta.buytryrate) / 100;

      if (_meta.buytrymult > 0) 
        price *= (_meta.buytrymult * generation);

      if (price < _meta.buytrymin)
        price = _meta.buytrymin;

      return price;
    }

    asset BranchMeta::buytryRevShare(const asset& revenue) {
      return (revenue * getMeta().buytryrate) / 100;
    }

    void BranchMeta::upsertBranchMeta(name owner, wflbrnchmeta meta) {
      checkCells(meta);
      checkRatios(meta);
      meta.owner = owner;
      if (_entKey >=1) {
        checkOwner(owner);
        checkNotUsedInBranches();
        meta.id = _entKey;
        update(owner, [&](auto& m) {
          m = meta;
        });
      }
      else {
        _entKey = nextPK();
        meta.id = _entKey;
        create(owner, [&](auto& m) {
          m = meta;
        });
      }
    }

    void BranchMeta::removeBranchMeta(name owner) {
      if (owner != _self) {
        checkOwner(owner);
        checkNotUsedInBranches();
      }
      remove();
    }

    void BranchMeta::checkIsMeta() {
      check(
        isEnt(),
        "No branch metadata found for id"
      );
    }

    void BranchMeta::checkOwner(name owner) {
        auto _meta = getMeta();
        check(
          owner == _meta.owner,
          "Branch metadata can be modified only by its owner"
        );
    }

    void BranchMeta::checkNotUsedInBranches() {
        //check for usage
        Branch::Branch branch(_self, 0);
        branch.checkBranchMetaNotUsed(_entKey);
    }

    void BranchMeta::checkCells(wflbrnchmeta meta) {
      check(
        meta.lvlreds >= 1  && meta.lvlgreens >= 1 && Const::lvlLength >= (meta.lvlreds + meta.lvlgreens + 1),
        "Please comply to level rules: lvlreds >= 1  AND lvlgreens >= 1 AND 16 >= (lvlreds + lvlgreens + 1)"
      );
    }

    void BranchMeta::checkRatios(wflbrnchmeta meta) {
      check(
        meta.slsrate <= 100 && meta.spltrate <= 100 && meta.stkrate <= 100 && 
        meta.tkrate <= 100 && meta.unjlrate <= 100 && meta.winnerrate <= 100,
        "Percent value can't exceed 100, please check rate fields"
      );
    }
  }
}
