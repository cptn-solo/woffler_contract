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
      bool startjailed = false;
      asset unjlmin = asset{0, Const::acceptedSymbol};
      uint8_t unjlrate = 0;
      asset buytrymin = asset{0, Const::acceptedSymbol};
      uint8_t buytryrate = 0;
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
      uint64_t maxlvlgen = 0; //maximum level generation (`nextlvl` limit)
      uint8_t takemult = 0; //multiplies level.generation which in turn multiplies `tkrate` up to 100
      uint8_t unljailmult = 0; //multiplies level.generation which in turn multiplies `unjlrate` up to 100
      uint8_t buytrymult = 0; //multiplies level.generation which in turn multiplies `unjlrate` up to 100

      uint64_t primary_key() const { return id; }
    } wflbrnchmeta;

    typedef multi_index<"brnchmeta"_n, wflbrnchmeta> brnchmetas;

    class DAO: public Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>  {
      
      public:

      DAO(brnchmetas& _brnchmetas, const uint64_t& idmeta):
        Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>::Accessor(_brnchmetas, idmeta) {}

      DAO(brnchmetas& _brnchmetas, const brnchmetas::const_iterator& itr):
        Accessor<brnchmetas, wflbrnchmeta, brnchmetas::const_iterator, uint64_t>::Accessor(_brnchmetas, itr) {}

      static uint64_t keyValue(const uint64_t& idmeta) {
        return idmeta;
      }
    };

    class BranchMeta: public Entity<brnchmetas, DAO, uint64_t, wflbrnchmeta> {

      public:

      BranchMeta(const name& self, const uint64_t& idmeta) : Entity<brnchmetas, DAO, uint64_t, wflbrnchmeta>(self, idmeta) {
      }
      
      wflbrnchmeta getMeta() {
        return getEnt();
      }
      
      asset nextPot(const asset& pot);
      asset splitPot(const asset& pot);
      asset takeAmount(const asset& pot, const uint64_t& generation, const uint64_t& winlevgen);
      
      asset stakeThreshold(const asset& pot);
      
      asset unjailPrice(const asset& pot, const uint64_t& generation);
      asset unjailRevShare(const asset& revenue);

      asset buytryPrice(const asset& pot, const uint64_t& generation);
      asset buytryRevShare(const asset& revenue);

      void checkIsMeta();
      void checkCells(const wflbrnchmeta& meta);
      void checkRatios(const wflbrnchmeta& meta);
      void checkOwner(const name& owner);
      void checkNotUsedInBranches();

      void upsertBranchMeta(const name& owner, wflbrnchmeta& meta);
      void removeBranchMeta(const name& owner);
      void removeBranchMeta();//debug
    };

  }
}
