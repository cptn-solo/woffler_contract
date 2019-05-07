#include <branchquest.hpp>

namespace Woffler {
  namespace BranchQuest {    
    BranchQuest::BranchQuest(name self, uint64_t idbrquest) : 
      Entity<brquests, DAO, uint64_t>(self, idbrquest) {}

    DAO::DAO(brquests& _brquests, uint64_t idbrquest): 
      Accessor<brquests, wflbrquest, brquests::const_iterator, uint64_t>::Accessor(_brquests, idbrquest) {}
  }
}
