#pragma once
#include <utils.hpp>
#include <constants.hpp>
#include <cell.hpp>

namespace woffler {
    using namespace eosio;
    using std::string;
    
    namespace wflLevel {
      class Level {
          public:
          
          Level(name self, uint64_t idlevel);
          
          uint64_t idlevel = 0;
          uint64_t idparent = 0;
          std::vector<uint8_t> redcells = {};
          std::vector<uint8_t> greencells = {};
          bool locked = true;
          bool root = true;
          uint64_t idbranch = 0;      
          uint64_t idchbranch = 0;
          uint64_t idmeta = 0;
          asset potbalance = asset{0, Const::acceptedSymbol};

          void checkLevel();        
          void checkLockedLevel();
          void checkUnlockedLevel();
          
          void createLevel(name payer, asset pot, uint64_t branchid, uint64_t metaid, uint8_t lvlreds, uint8_t lvllength);
          void generateRedCells(name payer, uint8_t lvlreds, uint8_t lvllength);
          void unlockTrial(name payer, uint8_t lvlgreens, uint8_t lvllength);
          void addPot(name payer, asset pot);

          Const::playerstate cellTypeAtPosition(uint8_t position);                

          //DEBUG:
          static void debugGenerateCells(name account, uint64_t num, uint8_t size, uint8_t maxval) {
            auto rnd = randomizer::getInstance(account, num);
            auto data = generateCells<uint8_t>(rnd, size, maxval);
            Utils::printVectorInt<uint8_t>(data);
          }

          void rmLevel();
          
          private:
          //branch levels
          TABLE wfllevel {
              uint64_t id;
              uint64_t idparent = 0;
              uint64_t idbranch;      
              uint64_t idchbranch = 0;
              uint64_t idmeta;
              asset potbalance = asset{0, Const::acceptedSymbol};
              std::vector<uint8_t> redcells;
              std::vector<uint8_t> greencells;
              bool locked = true;
              bool root = true;

              uint64_t primary_key() const { return id; }
              uint64_t get_idparent() const { return idparent; }
              uint64_t get_idbranch() const { return idbranch; }
          };
          typedef multi_index<"levels"_n, wfllevel,
              indexed_by<"byparent"_n, const_mem_fun<wfllevel, uint64_t, &wfllevel::get_idparent>>,//next level
              indexed_by<"bybranch"_n, const_mem_fun<wfllevel, uint64_t, &wfllevel::get_idbranch>>
          > levels;

          template<typename Lambda>
          void updateState(name payer, Lambda&& updater);

          template<class T>
          static std::vector<T> generateCells(randomizer& rnd, T size, T maxval) {
            std::vector<T> data(size);
            Cell::generator<T> generator(rnd, maxval, size);
            std::generate(data.begin(), data.end(), generator);
        
            return data;
          }

          name _self;
          levels _levels;
          uint64_t _idlevel;
      };
  }
}