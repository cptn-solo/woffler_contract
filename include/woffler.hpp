#pragma once
#include <utils.hpp>
#include <constants.hpp>

using namespace eosio;
using std::string;

CONTRACT woffler : public contract {
  public:
    using contract::contract;
    woffler(name receiver, name code, datastream<const char*> ds): 
      contract(receiver, code, ds) {}
    
    #pragma region ** Contract: **
    
    //signup new player with custom sales channel (via referral link)
    ACTION signup(name account, name channel);
    
    //forget player (without balance check yet, TBD!)
    ACTION forget(name account);

    //withdraw player's funds to arbitrary account (need auth by player)
    ACTION withdraw (name from, name to, asset amount, const string& memo);

    [[eosio::on_notify("eosio.token::transfer")]]
    void transferHandler(name from, name to, asset amount, string memo);
        
    using transferAction = action_wrapper<"transfer"_n, &woffler::transferHandler>;
    #pragma endregion
    
    #pragma region ** Sales channels (wflChannel): **

    //merge channel balance into owner's active balance
    ACTION chnmergebal(name owner);

    #pragma endregion
    
    #pragma region ** Branches (wflBranch): **
    
    //create meta for root branch(es) - active balance must cover at least.
    //owner pays for ram to avoid spamming via branch meta creation.
    //only owner can modify branch metadata.
    ACTION brnchmeta(name owner, 
      uint64_t id,
      asset unjlmin,
      uint8_t unjlrate,
      uint64_t unjlintrvl,
      uint8_t tkrate,
      uint64_t tkintrvl,
      uint8_t nxtrate,
      uint8_t spltrate,
      asset stkmin,
      uint8_t stkrate,
      asset potmin,
      uint8_t slsrate,
      string url,
      string name
    );
    
    //remove branch meta owned
    ACTION cleanbrmeta(name owner, uint64_t idmeta);
    
    //create root branch after meta is created/selected from existing
    ACTION branch(name owner, uint64_t idmeta, asset pot);
    
    #pragma endregion

  private:
    bool addBalance(name to, asset amount);
    void subBalance(name from, asset amount);
    bool clearAccount(name account, name scope);
    void upsertChannel(name owner);

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

    //branches for levels
    TABLE wflbranch {
      uint64_t id;
      uint64_t idparent;
      uint64_t idmeta;
      name winner;
      uint64_t generation;

      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"branches"_n, wflbranch> branches;

    //branch presets and metadata (applied for all subbranches)
    TABLE wflbrnchmeta {
      uint64_t id;
      name owner;
      asset unjlmin;
      uint8_t unjlrate;
      uint64_t unjlintrvl;
      uint8_t tkrate;
      uint64_t tkintrvl;
      uint8_t nxtrate;
      uint8_t spltrate;
      asset stkmin;
      uint8_t stkrate;
      asset potmin;
      uint8_t slsrate;
      string url;
      string name;

      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"brnchmeta"_n, wflbrnchmeta> brnchmetas;
};