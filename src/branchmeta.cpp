#include <branchmeta.hpp>

namespace Woffler {
  namespace BranchMeta {    
    BranchMeta::BranchMeta(name self, uint64_t idmeta) : 
      Entity<brnchmetas, DAO, uint64_t>(self, idmeta) {}

    DAO::DAO(brnchmetas& _brnchmetas, uint64_t idmeta): 
        Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>::Accessor(_brnchmetas, idmeta) {}
  }
}
