#pragma once
#include <entity.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  namespace Branch {
    //branches for levels
    typedef struct
    [[eosio::table("branches"), eosio::contract("woffler")]]
    wflbranch {
      uint64_t id;
      uint64_t idrootlvl = 0;
      uint64_t idparent = 0;
      uint64_t idmeta;
      name winner;
      uint64_t generation = 1;
      asset totalstake = asset{0, Const::acceptedSymbol};//appended each time stake added to avoid recalculation in runtime
      uint64_t lasttipid = 0;//any stake addition to the branch resets it's revtxid field to value of this field to prevent wrong tip allocations

      uint64_t primary_key() const { return id; }
      uint64_t get_idmeta() const { return idmeta; }
    } wflbranch;

    typedef multi_index<"branches"_n, wflbranch,
      indexed_by<"bymeta"_n, const_mem_fun<wflbranch, uint64_t, &wflbranch::get_idmeta>>
    > branches;

    //tipstkhldrs(uint64_t idbranch, asset amount, asset maxstake)
    typedef struct
    [[eosio::table("branchtips"), eosio::contract("woffler")]]
    wflbrtips {
      uint64_t id;//timestamp of the block/tx
      uint64_t idbranch; 
      asset base; //stake snapshot
      asset amount; //initial tip amount
      asset unclaimed; //unclaimed amount will be returned to the branch root (?) level pot after some timeout
      bool processed = false;
      
      uint64_t primary_key() const { return id; }
      bool get_processed() const { return processed; }
    } wflbrtips;  
    
    typedef multi_index<"branchtips"_n, wflbrtips, 
      indexed_by<"byprocessed"_n, const_mem_fun<wflbrtips, bool, &wflbranch::get_processed>>,
    > branchtips;


    class DAO: public Accessor<branches, wflbranch, branches::const_iterator, uint64_t>  {
      public:
      DAO(branches& _branches, uint64_t idbranch);
      DAO(branches& _branches, branches::const_iterator itr);
      static uint64_t keyValue(uint64_t idbranch) {
        return idbranch;
      }
    };

    class Branch: Entity<branches, DAO, uint64_t> {
      public:
      Branch(name self, uint64_t idbranch);
      
      wflbranch getBranch();
      uint64_t getRootLevel();
      
      void checkBranch();
      void checkStartBranch();
      void checkEmptyBranch();
      void checkBranchMetaNotUsed(uint64_t idmeta);

      void createBranch(name owner, uint64_t idmeta, asset pot);
      void createRootLevel(name owner);
      uint64_t addRootLevel(name owner, asset pot);
      void addStake(name owner, asset amount);
      void appendStake(name owner, asset amount);
      void setRootLevel(name payer, uint64_t idrootlvl);
      void setWinner(name player);      
      void deferRevenueShare(asset amount);
      void deferRevenueShare(asset amount, uint64_t idbranch);
      void allocateRevshare(uint64_t tipid);
      void rmBranch();
      
      bool isIndexedByMeta(uint64_t idmeta);
    };
  }
}