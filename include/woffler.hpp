#pragma once
#include <utils.hpp>
#include <constants.hpp>
#include <cell.hpp>

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

    #pragma region ** Player (wflPlayer): **

    //set current root branch for player and position at 1st level
    ACTION switchbrnch(name account, uint64_t idbranch);

    #pragma endregion

    #pragma region ** Sales channels (wflChannel): **

    //merge channel balance into owner's active balance
    ACTION chnmergebal(name owner);

    #pragma endregion
    
    #pragma region ** Branch presets (wflBranchMeta): **
    
    //create meta for root branch(es) - active balance must cover at least.
    //owner pays for ram to avoid spamming via branch meta creation.
    //only owner can modify branch metadata.
    ACTION brnchmeta(name owner, 
      uint64_t id,
      uint8_t lvllength,//min lvlgreens+lvlreds
      uint8_t lvlgreens,//min 1
      uint8_t lvlreds,//min 1
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
        
    #pragma endregion

    #pragma region ** Branches (wflBranch): **

    //create root branch with root level after meta is created/selected from existing
    //add pot value from owner's active balance to the root level's pot
    //register pot value as owner's stake in root branch created
    ACTION branch(name owner, uint64_t idmeta, asset pot);

    //link quest created earlier to the specified branch (see qstsetmeta)
    ACTION addquest(name owner, uint64_t idbranch, uint64_t idquest);
    
    //unlink quest from the specified branch
    ACTION rmquest(name owner, uint64_t idbranch, uint64_t idquest);

    #pragma endregion

    #pragma region ** Branch Stakes (wflStake): **
    
    //increase volume of root branch starting pot:
    //add amount to the root level's pot of the branch from owner's active balance
    //register amount as owner's stake in specified branch
    ACTION stkaddval(name owner, uint64_t idbranch, asset amount);

    //merge branch stake revenue into owner's active balance
    ACTION stktakervn(name owner, uint64_t idbranch);

    #pragma endregion

    #pragma region ** Levels (wflLevel): **

    //TEST action for cell generation debug 
    ACTION gencells(uint8_t size, uint8_t maxval);
    
    #pragma endregion

    #pragma region ** Quests (wflQuest): **
        
    //create/update owned quest metadata
    ACTION qstsetmeta(name owner, uint64_t idquest, 
      std::vector<uint64_t> hashes, 
      asset minprice, 
      asset maxprice, 
      string apiurl
    );

    //adjust owned quest balance using owner's active balance (add/remove to/from)
    ACTION qstsetbal(name owner, uint64_t idquest, asset amount);

    #pragma endregion

  private:

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

    //branch presets and metadata (applied for all subbranches)
    TABLE wflbrnchmeta {
      uint64_t id;
      name owner;
      uint8_t lvllength;//min lvlgreens+lvlreds
      uint8_t lvlgreens;//min 1
      uint8_t lvlreds;//min 1
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

    //branch stakeholders with accumulated revenue and stake
    TABLE wflstake {
      uint64_t id;
      uint64_t idbranch;
      name owner;
      asset stake;
      asset revenue;

      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"stakes"_n, wflstake> stakes;

    //branch levels
    TABLE wfllevel {
      uint64_t id;
      uint64_t idbranch;
      uint64_t idchbranch;
      asset potbalance;
      std::vector<uint8_t> redcells;
      std::vector<uint8_t> greencells;

      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"levels"_n, wfllevel> levels;

    //branch quests
    TABLE wflquest {
      uint64_t id;      
      name owner;
      asset balance;
      std::vector<uint64_t> hashes;
      asset minprice;
      asset maxprice;
      string apiurl;

      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"quests"_n, wflquest> quests;

    //branch-to-quest references
    TABLE wflbrquest {
      uint64_t id;
      uint64_t idbranch;
      uint64_t idquest;
      name owner;
      
      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"brquest"_n, wflbrquest> brquests;    
    
    bool addBalance(name to, asset amount);
    void subBalance(name from, asset amount);
    bool clearAccount(name account, name scope);
    void upsertChannel(name owner);

    template<class T>
    std::vector<T> generateCells(T size, T maxval);
    void addLevel(name owner, uint64_t idbranch, asset pot, const wflbrnchmeta& bmeta);
    void registerStake(name owner, uint64_t idbranch, asset amount);
};