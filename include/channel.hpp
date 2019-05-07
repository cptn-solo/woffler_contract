#include <entity.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  namespace Channel {
    //sales channels with user counter and current revenue balance available to merge into channel owner's balance
    typedef struct
    [[eosio::table("channels"), eosio::contract("woffler")]]
    wflchannel {
      name owner;
      uint64_t height = 1;
      asset balance = asset{0, Const::acceptedSymbol};

      uint64_t primary_key() const { return owner.value; }
    } wflchannel;
    
    typedef multi_index<"channels"_n, wflchannel> channels;

    class DAO: public Accessor<channels, wflchannel, channels::const_iterator, uint64_t>  {
      public:
      DAO(channels& _channels, uint64_t _ownerV);
      static uint64_t keyValue(name owner) {
          return owner.value;
      }
    };

    class Channel: Entity<channels, DAO, name> {
      public:
      Channel(name self, name owner);
      void upsertChannel(name payer);
      void subChannel(name payer);
    };
  }
}
