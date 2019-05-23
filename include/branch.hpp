#pragma once
#include <entity.hpp>
#include <branchmeta.hpp>
#include <stake.hpp>

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

      //revenue share fields:
      asset totalstake = asset{0, Const::acceptedSymbol};//appended each time stake added to avoid recalculation in runtime
      asset totalrvnue = asset{0, Const::acceptedSymbol}; //total tip amount
      asset winnerrvnue = asset{0, Const::acceptedSymbol};//total revenue paid to winner 
      asset parentrvnue = asset{0, Const::acceptedSymbol};//total revenue paid to parent branch 
      
      uint64_t tipprocessed = 0;

      uint64_t primary_key() const { return id; }
      uint64_t get_idmeta() const { return idmeta; }
      uint64_t get_tipprocessed() const { return tipprocessed; }
    } wflbranch;

    typedef multi_index<"branches"_n, wflbranch,
      indexed_by<"bymeta"_n, const_mem_fun<wflbranch, uint64_t, &wflbranch::get_idmeta>>,
      indexed_by<"byprocessed"_n, const_mem_fun<wflbranch, uint64_t, &wflbranch::get_tipprocessed>>
    > branches;

    class DAO: public Accessor<branches, wflbranch, branches::const_iterator, uint64_t>  {
      public:
      DAO(branches& _branches, uint64_t idbranch);
      DAO(branches& _branches, branches::const_iterator itr);
      static uint64_t keyValue(uint64_t idbranch) {
        return idbranch;
      }
    };

    class Branch: Entity<branches, DAO, uint64_t> {
      private:
      Stake::Stake stake;

      public:
      Branch(name self, uint64_t idbranch);
      
      wflbranch getBranch();
      uint64_t getRootLevel();
      
      void checkBranch();
      void checkStartBranch();
      void checkEmptyBranch();
      void checkBranchMetaNotUsed(uint64_t idmeta);

      void createBranch(name owner, uint64_t idmeta, asset pot);
      uint64_t createChildBranch(name owner, asset pot, uint64_t idparent);
      void addStake(name owner, asset amount);
      void appendStake(name owner, asset amount);
      void setRootLevel(name payer, uint64_t idrootlvl);
      void setWinner(name player);      
      void deferRevenueShare(asset amount);
      void deferRevenueShare(asset amount, uint64_t idbranch);
      void allocateRevshare();
      void rmBranch();
      
      bool isIndexedByMeta(uint64_t idmeta);
    };
  }
}