#include <branch.hpp>

namespace Woffler {
  namespace Branch {    
    Branch::Branch(name self, uint64_t idbranch) : 
      Entity<branches, DAO, uint64_t>(self, idbranch) {}

    DAO::DAO(branches& _branches, uint64_t idbranch): 
        Accessor<branches, wflbranch, branches::const_iterator, uint64_t>::Accessor(_branches, idbranch) {}

    void Branch::createBranch(name payer, uint64_t idmeta) {
      auto idbranch = nextPK();      
      create(payer, [&](auto& b) {
        b.id = idbranch;
        b.idmeta = idmeta;
      });
    }

    void Branch::setRootLevel(name payer, uint64_t idrootlvl) {
      update(payer, [&](auto& b) {
        b.idrootlvl = idrootlvl;
      });    
    }

    void Branch::checkBranch() {
      check(
        isEnt(),
        "No branch found for id"
      );
    }

    void Branch::checkStartBranch() {
      auto b = getEnt<wflbranch>();
      check(
        b.generation == 1,
        "Player can start only from root branch"
      );
      check(
        b.idrootlvl != 0,
        "Branch has no root level yet."
      );
    }  

    void Branch::checkEmptyBranch() {
      auto b = getEnt<wflbranch>();
      check(
        b.idrootlvl == 0,
        "Root level already exists"
      );
    }  

    void Branch::checkBranchMetaUsage(uint64_t idmeta) {
      check(
        isIndexedByMeta(idmeta),
        "Branch metadata is already used in branches."
      );
    }
    
    bool Branch::isIndexedByMeta(uint64_t idmeta) {
        auto idxbymeta = getIndex<"bymeta"_n>();
        auto itrbymeta = idxbymeta.find(idmeta);  
        return itrbymeta != idxbymeta.end();
    }    
  }
}