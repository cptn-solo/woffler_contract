#pragma once
#include <entity.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  namespace BranchMeta {
    //branch presets and metadata (applied for all subbranches)
    typedef struct
    [[eosio::table("brnchmetas"), eosio::contract("woffler")]]      
    wflbrnchmeta {
      uint64_t id;
      name owner;
      uint16_t lvlgreens;//min 1
      uint16_t lvlreds;//min 1
      asset unjlmin = asset{0, Const::acceptedSymbol};
      bool startjailed = 0;
      uint8_t unjlrate = 0;
      uint64_t unjlintrvl;
      uint8_t tkrate = 0;
      uint64_t tkintrvl = 0;
      uint8_t nxtrate = 90;
      uint8_t spltrate = 50;
      asset stkmin = asset{0, Const::acceptedSymbol};
      uint8_t stkrate = 0;
      asset potmin = asset{0, Const::acceptedSymbol};
      uint8_t slsrate = 0;
      uint8_t winnerrate = 0;
      string url;
      string name;

      uint64_t primary_key() const { return id; }
    } wflbrnchmeta;

    typedef multi_index<"brnchmeta"_n, wflbrnchmeta> brnchmetas;

    class DAO: public Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>  {
      public:
      DAO(brnchmetas& _brnchmetas, uint64_t idmeta);
      DAO(brnchmetas& _brnchmetas, brnchmetas::const_iterator itr);
      static uint64_t keyValue(uint64_t idmeta) {
        return idmeta;
      }
    };

    class BranchMeta: public Entity<brnchmetas, DAO, uint64_t> {
      public:
      BranchMeta(name self, uint64_t idmeta);

      wflbrnchmeta getMeta();

      asset nextPot(const asset& pot);
      asset splitPot(const asset& pot);
      asset takeAmount(const asset& pot);
      asset unjailPrice(const asset& pot);
      asset splitBetPrice(const asset& pot);
      
      void checkIsMeta();
      void checkCells(wflbrnchmeta meta);
      void checkOwner(name owner);
      void checkNotUsedInBranches();

      void upsertBranchMeta(name owner, wflbrnchmeta meta);
      void removeBranchMeta(name owner);
    };

  }
}
