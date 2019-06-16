#include <branchmeta.hpp>
#include <branch.hpp>

namespace Woffler {
  namespace BranchMeta {
    asset BranchMeta::nextPot(const asset& pot) {
      asset nxtPot = (pot * _entity.nxtrate) / 100;
      if (nxtPot < _entity.potmin)
        nxtPot = pot;
      return nxtPot;
    }

    asset BranchMeta::splitPot(const asset& pot) {
      //solved * SPLIT_RATE% >= STAKE_MIN?      
      auto minPot = (_entity.stkmin * 100) / _entity.spltrate;
      check(
        pot >= minPot,
        string("Reward pot of the current level is too small, should be at least ") + minPot.to_string().c_str()
      );

      auto splitAmount = (pot * _entity.spltrate) / 100;
      return splitAmount;
    }

    asset BranchMeta::takeAmount(const asset& levelpot, const uint64_t& generation, const asset& branchpot, const uint64_t& winlevgen) {
      check(branchpot.amount > 0, "Branch pot is empty");
      
      if (_entity.maxlvlgen > 0 && _entity.maxlvlgen == generation)//last level winner gets all remaining pot
        return branchpot;

      check(levelpot.amount > 0, "Reward pot is empty");

      auto reward = (levelpot * _entity.tkrate) / 100;
      
      check(reward.amount > 0, "Reward amount must be > 0");

      if (_entity.takemult > 0) 
        reward *= (_entity.takemult * generation); 

      if (reward > levelpot)
        return levelpot;

      return reward;
    }

    asset BranchMeta::stakeThreshold(const asset& pot) {
      //stake amounts derived from total branch pot, not current level's amount
      auto price = (pot * _entity.stkrate) / 100;
      if (price < _entity.stkmin)
        price = _entity.stkmin;

      return price;
    }

    asset BranchMeta::unjailPrice(const asset& pot, const uint64_t& generation) {
      auto price = (pot * _entity.unjlrate) / 100;
      if (_entity.unljailmult > 0)
        price *= (_entity.unljailmult * generation);

      if (price < _entity.unjlmin)
        price = _entity.unjlmin;

      return price;
    }

    asset BranchMeta::unjailRevShare(const asset& revenue) {
      return (revenue * _entity.unjlrate) / 100;
    }

    asset BranchMeta::buytryPrice(const asset& pot, const uint64_t& generation) {
      auto price = (pot * _entity.buytryrate) / 100;
      if (_entity.buytrymult > 0) 
        price *= (_entity.buytrymult * generation);

      if (price < _entity.buytrymin)
        price = _entity.buytrymin;

      return price;
    }

    asset BranchMeta::buytryRevShare(const asset& revenue) {
      return (revenue * _entity.buytryrate) / 100;
    }

    void BranchMeta::upsertBranchMeta(const name& owner, wflbrnchmeta& meta) {
      checkCells(meta);
      checkRatios(meta);
      meta.owner = owner;
      if (_entKey > 0) {
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
    void BranchMeta::removeBranchMeta() {
      remove();
    }
    
    void BranchMeta::removeBranchMeta(const name& owner) {
      checkOwner(owner);
      checkNotUsedInBranches();
      removeBranchMeta();
    }

    void BranchMeta::checkIsMeta() {
      check(
        isEnt(),
        "No branch metadata found for id"
      );
    }

    void BranchMeta::checkOwner(const name& owner) {
        check(
          owner == _entity.owner,
          "Branch metadata can be modified only by its owner"
        );
    }

    void BranchMeta::checkNotUsedInBranches() {
        //check for usage
        Branch::Branch branch(_self, 0);
        branch.checkBranchMetaNotUsed(_entKey);
    }

    void BranchMeta::checkCells(const wflbrnchmeta& meta) {
      check(
        meta.lvlreds >= 1  && meta.lvlgreens >= 1 && Const::lvlLength >= (meta.lvlreds + meta.lvlgreens + 1),
        "Please comply to level rules: lvlreds >= 1  AND lvlgreens >= 1 AND 16 >= (lvlreds + lvlgreens + 1)"
      );
    }

    void BranchMeta::checkRatios(const wflbrnchmeta& meta) {
      check(
        meta.slsrate <= 100 && meta.spltrate <= 100 && meta.stkrate <= 100 && 
        meta.tkrate <= 100 && meta.unjlrate <= 100 && meta.winnerrate <= 100,
        "Percent value can't exceed 100, please check rate fields"
      );
    }
  }
}
