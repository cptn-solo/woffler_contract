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
