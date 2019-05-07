#include <entity.hpp>
#include <cell.hpp>

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
      std::vector<uint8_t> redcells;
      std::vector<uint8_t> greencells;
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
      static uint64_t keyValue(uint64_t idlevel) {
          return idlevel;
      }
    };

    class Level: Entity<levels, DAO, uint64_t> {
      public:
      Level(name self, uint64_t idlevel);

      void checkLevel();        
      void checkLockedLevel();
      void checkUnlockedLevel();
      
      void createLevel(name payer, asset potbalance, uint64_t idbranch, uint64_t idmeta, uint8_t redcnt, uint8_t lvllength);
      void generateRedCells(name payer, uint8_t redcnt, uint8_t lvllength);
      void unlockTrial(name payer, uint8_t greencnt, uint8_t lvllength);
      void addPot(name payer, asset potbalance);

      Const::playerstate cellTypeAtPosition(uint8_t position);                
      
      template<class T>
      static std::vector<T> generateCells(randomizer& rnd, T size, T maxval) {
        std::vector<T> data(size);
        Cell::generator<T> generator(rnd, maxval, size);
        std::generate(data.begin(), data.end(), generator);

        return data;
      }

      //DEBUG:
      static void debugGenerateCells(name account, uint64_t num, uint8_t size, uint8_t maxval) {
        auto rnd = randomizer::getInstance(account, num);
        auto data = generateCells<uint8_t>(rnd, size, maxval);
        Utils::printVectorInt<uint8_t>(data);
      }
      
      //DEBUG:
      void rmLevel();
    };
  }
}