#include <quest.hpp>

namespace Woffler {
  namespace Quest {    
    Quest::Quest(name self, uint64_t idquest) : 
      Entity<quests, DAO, uint64_t>(self, idquest) {}

    DAO::DAO(quests& _quests, uint64_t idquest): 
        Accessor<quests, wflquest, quests::const_iterator, uint64_t>::Accessor(_quests, idquest) {}

    DAO::DAO(quests& _quests, quests::const_iterator itr): 
        Accessor<quests, wflquest, quests::const_iterator, uint64_t>::Accessor(_quests, itr) {}
  }
}
