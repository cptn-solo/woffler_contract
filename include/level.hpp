#pragma once
#include <entity.hpp>
#include <cell.hpp>
#include <branchmeta.hpp>
#include <branch.hpp>
#include <player.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;  
  namespace Level {
    //branch levels
    typedef struct
    [[eosio::table("levels"), eosio::contract("woffler")]]
    wfllevel {
      uint64_t id;
      uint64_t idparent = 0;
      uint64_t idbranch;
      uint64_t idchbranch = 0;
      uint64_t idmeta;
      uint16_t redcells;
      uint16_t greencells;
      bool locked = true;
      bool root = true;
      uint64_t generation = 1;//level sequential generation to simplify player's progress in gui
      asset potbalance = asset{0, Const::acceptedSymbol};//each operation must be reported to branch pot balance to enable clearance

      uint64_t primary_key() const { return id; }
      uint64_t get_idparent() const { return idparent; }
      uint64_t get_idbranch() const { return idbranch; }
    } wfllevel;

    typedef multi_index<"levels"_n, wfllevel,
      indexed_by<"byparent"_n, const_mem_fun<wfllevel, uint64_t, &wfllevel::get_idparent>>,//next level
      indexed_by<"bybranch"_n, const_mem_fun<wfllevel, uint64_t, &wfllevel::get_idbranch>>
    > levels;

    class DAO: public Accessor<levels, wfllevel, levels::const_iterator, uint64_t>  {
      public:
      DAO(levels& _levels, const uint64_t& idlevel):
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, idlevel) {}

      DAO(levels& _levels, const levels::const_iterator& itr):
        Accessor<levels, wfllevel, levels::const_iterator, uint64_t>::Accessor(_levels, itr) {}
    
      static uint64_t keyValue(const uint64_t& idlevel) {
        return idlevel;
      }
    };

    class Level: public Entity<levels, DAO, uint64_t, wfllevel> {
      protected:
      wfllevel _level;
      BranchMeta::wflbrnchmeta _meta;
      Branch::wflbranch _branch;

      void fetchContext() {
        if (isEnt()) {
          _level = getLevel();
          
          meta.fetchByKey(_level.idmeta);
          _meta = meta.getMeta();
          
          branch.fetchByKey(_level.idbranch);
          _branch = branch.getBranch();
        }
      }

      public:
      Level(const name& self, const uint64_t& idlevel) : Entity<levels, DAO, uint64_t, wfllevel>(self, idlevel), meta(self, 0), branch(self, 0) {
        fetchContext();
      }

      Level(name self) : Level(self, 0) {}

      wfllevel getLevel() {
        return getEnt();
      }

      BranchMeta::BranchMeta meta;
      Branch::Branch branch;

      void checkLockedLevel();
      void checkUnlockedLevel();

      uint64_t createLevel(const name& payer, const uint64_t& idbranch, const uint64_t& idparent, const uint64_t& generation, const uint64_t& idmeta, const bool& root, const asset& pot);
      void addPot(const name& payer, const asset& pot);
      void unlockLevel(const name& owner);
      void generateRedCells(const name& payer);
      bool unlockTrial(const name& payer);

      Const::playerstate cellTypeAtPosition(const uint8_t& position);

      void regenCells(const name& owner);//debug mostly

      template<typename T>
      static uint16_t generateCells(randomizer& rnd, T size) {
        uint16_t data = 0;
        Cell::generator<T> generator(rnd, 16, size);//max length is 16 as we are using uint16_t
        for (size_t i = 0; i < size; i++)
          data += (1<<generator());        
        return data;
      }

      //DEBUG:
      static void debugGenerateCells(const name& account, const uint64_t& num, const uint8_t& size) {
        auto rnd = randomizer::getInstance(account, num);
        auto data = generateCells<uint8_t>(rnd, size);
        print_f("cells data: % \n", std::to_string(data));
      }

      //DEBUG:
      void rmLevel();
    };

    class PlayerLevel: public Level {      
      private:
      Player::Player player;
      Player::wflplayer _player;

      void cutRevenueShare(asset& revenue, const Const::revenuetype& revtype);

      public:
      PlayerLevel(name self, name account) : Level(self), player(self, account) {
        _player = player.getPlayer();
        fetchByKey(_player.idlevel);
        fetchContext();
      }
      
      void tryTurn();
      void commitTurn();
      void cancelTake();
      void claimSafe();
      void claimRed();
      void claimTake();
      void nextLevel();
      void takeReward();
      void unjailPlayer();
      void splitLevel();
      void buyRetries();
    };
  }
}