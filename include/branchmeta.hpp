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
      uint8_t unjlrate;
      uint64_t unjlintrvl;
      uint8_t tkrate;
      uint64_t tkintrvl;
      uint8_t nxtrate;
      uint8_t spltrate;
      asset stkmin = asset{0, Const::acceptedSymbol};
      uint8_t stkrate;
      asset potmin = asset{0, Const::acceptedSymbol};
      uint8_t slsrate;
      uint8_t winnerrate;
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

    class BranchMeta: Entity<brnchmetas, DAO, uint64_t> {
      public:
      BranchMeta(name self, uint64_t idmeta);

      wflbrnchmeta getMeta();
      
      void checkIsMeta();
      void checkCells(wflbrnchmeta meta);
      void checkOwner(name owner);
      void checkNotUsedInBranches();

      void upsertBranchMeta(name owner, wflbrnchmeta meta);
      void removeBranchMeta(name owner);
    };

  }
}
