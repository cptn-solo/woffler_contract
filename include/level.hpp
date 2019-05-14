#pragma once
#include <entity.hpp>
#include <cell.hpp>
#include <branchmeta.hpp>
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
      asset potbalance = asset{0, Const::acceptedSymbol};
      uint16_t redcells;
      uint16_t greencells;
      bool locked = true;
      bool root = true;

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
      DAO(levels& _levels, uint64_t idlevel);
      DAO(levels& _levels, levels::const_iterator itr);
      static uint64_t keyValue(uint64_t idlevel) {
        return idlevel;
      }
    };

    class Level: public Entity<levels, DAO, uint64_t> {
      protected:
      void setIdLevel(uint64_t idlevel);
      
      public:
      Level(name self, uint64_t idlevel);
      Level(name self);

      wfllevel getLevel();

      void checkLevel();
      void checkLockedLevel();
      void checkRootLevel();
      void checkUnlockedLevel();

      uint64_t createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, uint64_t idmeta, uint8_t redcnt);
      uint64_t createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idparent, BranchMeta::wflbrnchmeta meta);
      void unlockRootLevel(name owner);
      void generateRedCells(name payer, uint8_t redcnt);
      void unlockTrial(name payer, uint8_t greencnt);
      void addPot(name payer, asset potbalance);

      Const::playerstate cellTypeAtPosition(uint8_t position);

      void regenCells(name owner);//debug mostly

      template<typename T>
      static uint16_t generateCells(randomizer& rnd, T size) {
        uint16_t data = 0;
        Cell::generator<T> generator(rnd, 16, size);//max length is 16 as we are using uint16_t
        for (size_t i = 0; i < size; i++)
          data += (1<<generator());        
        return data;
      }

      //DEBUG:
      static void debugGenerateCells(name account, uint64_t num, uint8_t size) {
        auto rnd = randomizer::getInstance(account, num);
        auto data = generateCells<uint8_t>(rnd, size);
        print_f("cells data: % \n", std::to_string(data));
      }

      //DEBUG:
      void rmLevel();
    };

    class PlayerLevel: public Level {      
      public:
      PlayerLevel(name self, name account);

      void nextLevel();
      void unjailPlayer();
      
      private:
      Player::Player player;

    };
  }
}