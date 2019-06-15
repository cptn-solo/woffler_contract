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
      asset revenue = asset{0, Const::acceptedSymbol};//added from branch each claimRevenue the same time amount added goes to owner's active balance

      uint64_t primary_key() const { return id; }
      uint64_t get_idbranch() const { return idbranch; }
      uint64_t get_owner() const { return owner.value; }
      uint128_t get_ownedbrnch() const { return Utils::combineIds(owner.value, idbranch); }
    } wflstake;

    typedef multi_index<"stakes"_n, wflstake,
      indexed_by<"bybranch"_n, const_mem_fun<wflstake, uint64_t, &wflstake::get_idbranch>>,
      indexed_by<"bybowner"_n, const_mem_fun<wflstake, uint64_t, &wflstake::get_owner>>,
      indexed_by<"byownedbrnch"_n, const_mem_fun<wflstake, uint128_t, &wflstake::get_ownedbrnch>>
    > stakes;
  
    class DAO: public Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>  {
      public:
      DAO(stakes& _stakes, const uint64_t& idstake): 
        Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, idstake) {}
      
      DAO(stakes& _stakes, const stakes::const_iterator& itr): 
        Accessor<stakes, wflstake, stakes::const_iterator, uint64_t>::Accessor(_stakes, itr) {}
    
      static uint64_t keyValue(const uint64_t& idstake) {
        return idstake;
      }
    };

    class Stake: Entity<stakes, DAO, uint64_t, wflstake> {
      public:
      Stake(const name& self, const uint64_t& idstake) : 
        Entity<stakes, DAO, uint64_t, wflstake>(self, idstake) {}

      Stake(const name& self) : 
        Entity<stakes, DAO, uint64_t, wflstake>(self, 0) {}

      void registerStake(const name& owner, const uint64_t& idbranch, const asset& amount);
      void branchStake(const name& owner, const uint64_t& idbranch, asset& total, asset& owned);
      void claimRevenue(const name& owner, const uint64_t& idbranch);

      void rmStake();

      void checkIsStakeholder(const name& owner, const uint64_t& idbranch);
    };

  }
}