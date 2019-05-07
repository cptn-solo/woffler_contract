#include <entity.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  namespace Quest {
    //branch quests
    typedef struct
    [[eosio::table("quests"), eosio::contract("woffler")]]      
    wflquest {
      uint64_t id;      
      name owner;
      asset balance = asset{0, Const::acceptedSymbol};
      std::vector<uint64_t> hashes;
      asset minprice = asset{0, Const::acceptedSymbol};
      asset maxprice = asset{0, Const::acceptedSymbol};
      string apiurl;

      uint64_t primary_key() const { return id; }
    } wflquest;

    typedef multi_index<"quests"_n, wflquest> quests;

    class DAO: public Accessor<quests, wflquest, quests::const_iterator, uint64_t>  {
      public:
      DAO(quests& _quests, uint64_t idquest);
      static uint64_t keyValue(uint64_t idquest) {
        return idquest;
      }
    };

    class Quest: Entity<quests, DAO, uint64_t> {
      public:
      Quest(name self, uint64_t idquest);
    };
  }
}


