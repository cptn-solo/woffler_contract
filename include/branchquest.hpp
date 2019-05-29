#include <entity.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  namespace BranchQuest {
    //branch-to-quest references
    typedef struct
    [[eosio::table("brquests"), eosio::contract("woffler")]]      
    wflbrquest {
      uint64_t id;
      uint64_t idbranch;
      uint64_t idquest;
      name owner;
      
      uint64_t primary_key() const { return id; }
    } wflbrquest;

    typedef multi_index<"brquest"_n, wflbrquest> brquests;  

    class DAO: public Accessor<brquests, wflbrquest, brquests::const_iterator, uint64_t>  {
      public:
      DAO(brquests& _brquests, uint64_t idbrquest);
      DAO(brquests& _brquests, brquests::const_iterator itr);
      static uint64_t keyValue(uint64_t idbrquest) {
        return idbrquest;
      }
    };

    class BranchQuest: Entity<brquests, DAO, uint64_t> {
      public:
      BranchQuest(name self, uint64_t idbrquest);
    };
  }
}


