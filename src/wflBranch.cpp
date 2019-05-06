#include <utils.hpp>
#include <constants.hpp>
#include <wflBranch.hpp>

namespace Woffler {
  namespace Branch {    
    Branch::Branch(name self, uint64_t idbranch) : Entity<branches, DAO, uint64_t>(self, idbranch) {
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
        _dao->isIndexedByMeta(idmeta),
        "Branch metadata is already used in branches."
      );
    }

    void Branch::createBranch(name payer, uint64_t metaid) {
      auto idbranch = nextPK();      
      create(payer, [&](auto& b) {
        b.id = idbranch;
        b.idmeta = idmeta;
      });
    }

    void Branch::setRootLevel(name payer, uint64_t rootlvlid) {
      update(payer, [&](auto& b) {
        b.idrootlvl = idrootlvl;
      });    
    }

  }
}