#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <constants.hpp>

using namespace eosio;
using std::string;

CONTRACT woffler : public contract {
  public:
    using contract::contract;
    woffler(name receiver, name code, datastream<const char*> ds): 
      contract(receiver, code, ds) {}
    
    //** Contract: **//
    
    //signup new player with custom sales channel (via referral link)
    ACTION signup(name account, name channel);
    
    //forget player (without balance check yet, TBD!)
    ACTION forget(name account);

    //withdraw player's funds to arbitrary account (need auth by player)
    ACTION withdraw (name from, name to, asset amount, const string& memo);

    [[eosio::on_notify("eosio.token::transfer")]]
    void transferHandler(name from, name to, asset amount, string memo);
        
    using transferAction = action_wrapper<"transfer"_n, &woffler::transferHandler>;

    //** Sales channels: **//

    //merge channel balance into owner's active balance
    ACTION chnmergebal(name owner);

  private:
    bool addBalance(name to, asset amount);
    void subBalance(name from, asset amount);
    bool clearAccount(name account, name scope);

    //players with there balances and in-game state
    TABLE wflplayer {
      name account;
      name channel;
      uint64_t idlvl;
      asset activebalance;
      asset vestingbalance;
      uint8_t tryposition;
      uint8_t currentposition;
      uint8_t triesleft;
      uint8_t levelresult;
      uint64_t resulttimestamp;
      
      uint64_t primary_key() const { return account.value; }
    };
    typedef multi_index<"players"_n, wflplayer> players;        
    
    //sales channels with user counter and current revenue balance available to merge into channel owner's balance
    TABLE wflchannel {
      name owner;
      uint64_t height;
      asset balance;
      
      uint64_t primary_key() const { return owner.value; }
    };
    typedef multi_index<"channels"_n, wflchannel> channels;        

};