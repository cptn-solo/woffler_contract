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
      uint64_t generation = 1;
      name winner;
      uint64_t winlevel = 0;
      uint64_t winlevgen = 0;

      //revenue share fields:
      asset totalstake = asset{0, Const::acceptedSymbol};//appended each time stake added to avoid recalculation in runtime
      asset totalrvnue = asset{0, Const::acceptedSymbol}; //total tip amount
      asset winnerrvnue = asset{0, Const::acceptedSymbol};//total revenue paid to winner 
      asset parentrvnue = asset{0, Const::acceptedSymbol};//total revenue paid to parent branch 
      
      uint64_t tipprocessed = 1;//initialize branch without need to be processed

      asset potbalance = asset{0, Const::acceptedSymbol};//potbalance shared across branch levels via `nextrate`
      uint32_t openchildcnt = 0;//open child branches
      uint32_t closed = 0;//timestamp of close event (pot emptied)

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
      DAO(branches& _branches, const uint64_t& idbranch):
        Accessor<branches, wflbranch, branches::const_iterator, uint64_t>::Accessor(_branches, idbranch) {}

      DAO(branches& _branches, const branches::const_iterator& itr):
        Accessor<branches, wflbranch, branches::const_iterator, uint64_t>::Accessor(_branches, itr) {}
      
      static uint64_t keyValue(const uint64_t& idbranch) {
        return idbranch;
      }
    };

    class Branch: public Entity<branches, DAO, uint64_t, wflbranch> {
      
      private:

      Stake::Stake stake;
      wflbranch _branch;

      public:

      Branch(const name& self, const uint64_t& idbranch) : Entity<branches, DAO, uint64_t, wflbranch>(self, idbranch), stake(self, 0) {
        if (isEnt())
          _branch = getBranch();
      }

      wflbranch getBranch() {
        return getEnt();
      }

      uint64_t getRootLevel() {
        auto b = getEnt();
        return b.idrootlvl;
      }
      
      void checkBranch();
      void checkStartBranch();
      void checkNotClosed();
      void checkEmptyBranch();
      void checkBranchMetaNotUsed(const uint64_t& idmeta);

      void createBranch(const name& owner, const uint64_t& idmeta, const asset& pot);
      uint64_t createChildBranch(const name& owner, const uint64_t& pidbranch, const uint64_t& pidlevel, const asset& pot);
      void addPot(const name& payer, const asset& pot);
      void subPot(const name& payer, const asset& take);
      void addStake(const name& owner, const asset& amount);
      void appendStake(const name& owner, const asset& amount);
      void setRootLevel(const name& payer, const uint64_t& idrootlvl, const uint64_t& generation);
      void updateTreeDept(const name& payer, const uint64_t& idlevel, const uint64_t& generation);
      void setWinner(const name& player);      
      void deferRevenueShare(const asset& amount);
      void deferRevenueShare(const asset& amount, const uint64_t& idbranch);
      void allocateRevshare();
      void closeBranch();
      void rmBranch();
      
      bool isIndexedByMeta(const uint64_t& idmeta);
    };
  }
}