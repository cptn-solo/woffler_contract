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
      //decide on new level's pot
      asset nxtPot = (pot * _meta.nxtrate) / 100;
      if (nxtPot < _meta.potmin)
        nxtPot = pot;
      return nxtPot;
    }

    asset BranchMeta::splitPot(const asset& pot) {
      auto _meta = getMeta();
      //solved * SPLIT_RATE% * STAKE_RATE% > STAKE_MIN?
      auto minSplitAmount = (_meta.stkmin * 100) / _meta.stkrate;
      auto mitSplittablePotAmount = (minSplitAmount * 100) / _meta.spltrate;
      check(
        pot >= mitSplittablePotAmount,
        string("Reward pot of the current level is too small, should be at least ") + mitSplittablePotAmount.to_string().c_str()
      );

      auto splitAmount = (pot * _meta.spltrate) / 100;
      return splitAmount;
    }

    asset BranchMeta::takeAmount(const asset& pot) {
      auto _meta = getMeta();
      auto reward = (pot * _meta.tkrate) / 100;

      //solved * (100-TAKE_RATE)% > POT_MIN ?
      check(reward.amount > 0, "Reward amount must be > 0");
      check(
        (pot - reward) >= _meta.potmin,
        string("Level pot is insufficient for reward, must be at least ")+_meta.potmin.to_string().c_str()+string(" after reward paid.\n")
      );

      return reward;
    }

    asset BranchMeta::unjailPrice(const asset& pot) {
      auto _meta = getMeta();
      auto unjailAmount = (pot * _meta.unjlrate) / 100;
      if (unjailAmount < _meta.unjlmin)
        unjailAmount = _meta.unjlmin;

      return unjailAmount;
    }

    asset BranchMeta::unjailRevShare(const asset& revenue) {
      auto _meta = getMeta();
      auto share = (revenue * _meta.unjlrate) / 100;
      return share;
    }

    asset BranchMeta::splitBetPrice(const asset& pot) {
      auto _meta = getMeta();
      auto betPrice = (pot * _meta.stkrate) / 100;
      if (betPrice < _meta.stkmin)
        betPrice = _meta.stkmin;

      return betPrice;
    }

    asset BranchMeta::splitBetRevShare(const asset& revenue) {
      auto _meta = getMeta();
      auto share = (revenue * _meta.stkrate) / 100;
      return share;
    }

    void BranchMeta::upsertBranchMeta(name owner, wflbrnchmeta meta) {
      checkCells(meta);
      if (_entKey >=1) {
        checkOwner(owner);
        checkNotUsedInBranches();

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
      checkOwner(owner);
      checkNotUsedInBranches();

      remove();
    }

    void BranchMeta::checkIsMeta() {
      check(
        isEnt(),
        "No branch metadata found for id"
      );
    }

    void BranchMeta::checkOwner(name owner) {
        auto m = getEnt<wflbrnchmeta>();
        check(
          owner == m.owner,
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
        string("Please comply to level rules: lvlreds >= 1  AND lvlgreens >= 1 AND 16 >= (lvlreds + lvlgreens + 1)")
      );
    }
  }
}
