#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <constants.hpp>

using namespace eosio;

using std::string;

CONTRACT woffler : public contract {
  public:
    using contract::contract;
    woffler(name receiver, name code, datastream<const char*> ds): contract(receiver, code, ds), _players(receiver, code.value) {}

    ACTION signup(name account);
    
    void deposit(name from, name to, asset amount, string memo);
  
  private:
    TABLE wflplayer {
      name account;
      uint64_t idLvl;
      asset activebalance;
      asset vestingbalance;
      uint64_t tryposition;
      uint64_t currentposition;
      uint64_t triesleft;
      uint64_t levelresult;
      uint64_t resulttimestamp;
      
      uint64_t primary_key() const { return account.value; }
    };
    typedef multi_index<"players"_n, wflplayer> playerstable;
    
    playerstable _players;
};