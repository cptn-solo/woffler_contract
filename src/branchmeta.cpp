#include <branchmeta.hpp>
#include <branch.hpp>

namespace Woffler {
  namespace BranchMeta {
    BranchMeta::BranchMeta(name self, name owner, uint64_t idmeta) :
      Entity<brnchmetas, DAO, uint64_t>(self, idmeta), _meta{} {
        this->_owner = owner;
      }

    BranchMeta::BranchMeta(name self, name owner, wflbrnchmeta meta) :
      Entity<brnchmetas, DAO, uint64_t>(self, meta.id), _meta{meta} {
        this->_owner = owner;
      }

    DAO::DAO(brnchmetas& _brnchmetas, uint64_t idmeta):
        Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>::Accessor(_brnchmetas, idmeta) {}

    void BranchMeta::upsertBranchMeta() {
      checkCells();
      if (_meta.id >= 1) {
        checkOwner();
        checkNotUsedInBranches();

        update(_owner, [&](auto& m) {
          m = _meta;
        });
      }
      else {
        _entKey = nextPK();
        _meta.id = _entKey;
        create(_owner, [&](auto& m) {
          m = _meta;
        });
      }
    }

    void BranchMeta::removeBranchMeta() {
      checkOwner();
      checkNotUsedInBranches();

      remove();
    }

    void BranchMeta::checkIsMeta() {
      check(
        isEnt(),
        "No branch metadata found for id"
      );
    }

    void BranchMeta::checkOwner() {
        auto m = getEnt<wflbrnchmeta>();
        check(
          _owner == m.owner,
          "Branch metadata can be modified only by its owner"
        );
    }

    void BranchMeta::checkNotUsedInBranches() {
        //check for usage
        Branch::Branch branch(_self, 0);
        branch.checkBranchMetaNotUsed(_entKey);
    }

    void BranchMeta::checkCells() {
      check(
        _meta.lvllength <= Const::maxLvlLength && _meta.lvlreds >= 1  && _meta.lvlgreens >= 1 && _meta.lvllength >= (_meta.lvlreds + _meta.lvlgreens + 1),
        string("Please comply to level rules: lvllength <= ") + std::to_string(Const::maxLvlLength) + string(" AND lvlreds >= 1  AND lvlgreens >= 1 AND lvllength >= (lvlreds + lvlgreens + 1)")
      );
    }
  }
}
