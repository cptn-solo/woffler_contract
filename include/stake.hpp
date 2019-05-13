#pragma once
#include <entity.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  namespace Stake {
    //branch stakeholders with accumulated revenue and stake
    typedef struct
    [[eosio::table("stakes"), eosio::contract("woffler")]]      
    wflstake {
      uint64_t id;
      uint64_t idbranch;
      name owner;
      asset stake = asset{0, Const::acceptedSymbol};
      asset revenue = asset{0, Const::acceptedSymbol};

      uint64_t primary_key() const { return id; }
      uint64_t get_idbranch() const { return idbranch; }
      uint128_t get_ownedbrnch() const { return Utils::combineIds(owner.value, idbranch); }
    } wflstake;

    typedef multi_index<"stakes"_n, wflstake,
      indexed_by<"bybranch"_n, const_mem_fun<wflstake, uint64_t, &wflstake::get_idbranch>>,
      indexed_by<"byownedbrnch"_n, const_mem_fun<wflstake, uint128_t, &wflstake::get_ownedbrnch>>
    > stakes;

    class DAO: public Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>  {
      public:
      DAO(stakes& _stakes, uint64_t idstake);
      DAO(stakes& _stakes, stakes::const_iterator itr);
      static uint64_t keyValue(uint64_t idstake) {
        return idstake;
      }
    };

    class Stake: Entity<stakes, DAO, uint64_t> {
      public:
      Stake(name self, uint64_t idstake);
      
      void addStake(name owner, uint64_t idbranch, asset amount);
      void registerStake(name owner, uint64_t idbranch, asset amount);
      void branchStake(name owner, uint64_t idbranch, asset& total, asset& owned);
      void rmStake();

      void checkIsStakeholder(name owner, uint64_t idbranch);
    };

  }
}