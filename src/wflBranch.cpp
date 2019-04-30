#include <wflBranch.hpp>

namespace woffler {
  namespace wflBranch {
    Branch::Branch(name _self, uint64_t _idbranch) : _branches(_self, _self.value) {
      this->_self = _self;
      this->_idbranch = _idbranch;
      this->_branches = _branches;
    }

    template<typename Lambda>
    void Branch::updateState(name payer, Lambda&& updater) {
      auto branch = _branches.find(_idbranch);
      _branches.modify(*branch, payer, std::forward<Lambda&&>(updater)); 
    }

    void Branch::checkBranch() {
      auto branch = _branches.find(_idbranch);
      check(
        branch != _branches.end(),
        "No branch found for id"
      );
      idbranch = branch->id;
      idrootlvl = branch->idrootlvl;
      idparent = branch->idparent;
      idmeta = branch->idmeta;
      winner = branch->winner;
      generation = branch->generation;
    }

    void Branch::checkStartBranch() {
      checkBranch();
      check(
        generation == 1,
        "Player can start only from root branch"
      );
      check(
        idrootlvl != 0,
        "Branch has no root level yet."
      );
    }  

    void Branch::checkEmptyBranch() {
      checkBranch();
      check(
        idrootlvl == 0,
        "Root level already exists"
      );
    }  
    
    void Branch::checkBranchMetaUsage(uint64_t idmeta) {
      auto idxbymeta = _branches.get_index<name("bymeta")>();
      auto itrbymeta = idxbymeta.find(idmeta);  

      check(
        itrbymeta == idxbymeta.end(),
        "Branch metadata is already used in branches."
      );
    }

    void Branch::createBranch(name payer, uint64_t metaid) {
      auto branchid = Utils::nextPrimariKey(_branches.available_primary_key());
      idbranch = branchid;
      _idbranch = branchid;
      idmeta = metaid;
      
      _branches.emplace(payer, [&](auto& b) {
        b.id = idbranch;
        b.idmeta = idmeta;
      });
    }

    void Branch::setRootLevel(name payer, uint64_t rootlvlid) {
      idrootlvl = rootlvlid;
      updateState(payer, [&](auto& b) {
        b.idrootlvl = idrootlvl;
      });    
    }
  }  
}