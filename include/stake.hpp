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
      uint64_t lasttipid = 0; //id of last rev.share tx to filter out processed allocations. 
                            //claim should be done from lowest to highest revtxid values to avoid 'missing' 
                            //unclaimed amount - claimtip action is applicable only for tip id's lower than saved in this field
                            //and can't be undone.
                            //this is a drawback to not implement ledger to produce/store [branch x stakeowner x tip] number of records

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

      void registerStake(name owner, uint64_t idbranch, asset amount, uint64_t revtxid);
      void branchStake(name owner, uint64_t idbranch, asset& total, asset& owned);
      asset branchStake(uint64_t idbranch);
      void claimTip(name owner, uint64_t txid);
      void rmStake();

      void checkIsStakeholder(name owner, uint64_t idbranch);
    };

  }
}