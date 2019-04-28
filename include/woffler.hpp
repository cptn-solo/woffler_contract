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
    ACTION switchbrnch(name player, uint64_t idbranch);

    //use try to change position in current level from safe to green. last try will change position automatically
    ACTION tryturn(name player);
    
    //commit position change in current level
    ACTION committurn(name player);

    //commit player's position after turn result "red cell" (position player to prev. level's zero)
    ACTION claimred(name player);

    //reset player's GREEN position to SAFE (current level's zero cell) if a player don't want to continue trial of splitting branch or extending it
    ACTION claimgreen(name player);

    //reset player's TAKE position to SAFE (current level's zero cell) after TAKE level result timestamp expired
    ACTION claimtake(name player);

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

    //create root branch after meta is created/selected from existing
    //register pot value as owner's stake in root branch created
    ACTION branch(name owner, uint64_t idmeta, asset pot);

    //create root level with all branch stake (from all owners)
    //generate cells for root level
    ACTION rootlvl(name owner, uint64_t idbranch);

    //link quest created earlier to the specified branch (see qstsetmeta)
    ACTION addquest(name owner, uint64_t idbranch, uint64_t idquest);
    
    //unlink quest from the specified branch
    ACTION rmquest(name owner, uint64_t idbranch, uint64_t idquest);
    
    //DEBUG actions for branch generation debug 
    ACTION setrootlvl(name owner, uint64_t idbranch, uint64_t idrootlvl);

    #pragma endregion

    #pragma region ** Branch Stakes (wflStake): **
    
    //increase volume of root branch starting pot:
    //cut amount from owner's active balance
    //register amount as owner's stake in specified branch
    //if branch already has a root lvl, add amount to its pot 
    ACTION stkaddval(name owner, uint64_t idbranch, asset amount);

    //merge branch stake revenue into owner's active balance
    ACTION stktakervn(name owner, uint64_t idbranch);

    #pragma endregion

    #pragma region ** Levels (wflLevel): **
    
    //generate cells for a given level and mark level unlocked if compatible green/red set found
    ACTION unlocklvl(name owner, uint64_t idlevel);
    
    //position player to the next level
    //if not yet exists - initialize new locked level in current branch 
    //split pot according to level's branch metadata(`nxtrate`), 
    //make the player a branch winner
    //as new level is locked, winner have 3 tries to unlock it, if no luck - zero-ed in current level
    ACTION nextlvl(name player);

    //split level's pot according to level's branch metadata (`tkrate`) and reward player (vesting balance update)
    //player wait untill the end of `tkintrvl` set with level result upon `takelvl`
    //player calls `claimtake` to move further after `tkintrvl` expires - then zero-ed in current level
    ACTION takelvl(name player);

    //make subbranch with locked root level
    //split level's pot according to level's branch metadata (`spltrate`, `potmin`)
    //make the player a stakeholder of new subbranch, share is defined by level's branch metadata (`stkrate`, `stkmin`)
    //as new level is locked, splitter have 3 tries to unlock it, if no luck - zero-ed in current level
    ACTION splitlvl(name player);

    //if no free unlock retries left, player can bet for split from his active balance to reset retries count  
    //bet amount is calculated according to level's branch metadata (`stkrate`, `stkmin`)
    ACTION splitbet(name player);

    //DEBUG actions for level generation debug 
    ACTION gencells(name account, uint8_t size, uint8_t maxval);
    ACTION regencells(name owner, uint64_t idlevel);
    ACTION rmlevel(name owner, uint64_t idlevel);
    
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
      uint64_t idlvl = 0;
      asset activebalance = asset{0, Const::acceptedSymbol};
      asset vestingbalance = asset{0, Const::acceptedSymbol};
      uint8_t tryposition = 0;
      uint8_t currentposition = 0;
      uint8_t triesleft = 0;
      uint8_t levelresult = Const::playerstate::INIT;
      uint32_t resulttimestamp;
      
      uint64_t primary_key() const { return account.value; }
    };
    typedef multi_index<"players"_n, wflplayer> players;        
    
    //sales channels with user counter and current revenue balance available to merge into channel owner's balance
    TABLE wflchannel {
      name owner;
      uint64_t height = 1;
      asset balance = asset{0, Const::acceptedSymbol};
      
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
      asset unjlmin = asset{0, Const::acceptedSymbol};
      uint8_t unjlrate;
      uint64_t unjlintrvl;
      uint8_t tkrate;
      uint64_t tkintrvl;
      uint8_t nxtrate;
      uint8_t spltrate;
      asset stkmin = asset{0, Const::acceptedSymbol};
      uint8_t stkrate;
      asset potmin = asset{0, Const::acceptedSymbol};
      uint8_t slsrate;
      string url;
      string name;

      uint64_t primary_key() const { return id; }
    };
    typedef multi_index<"brnchmeta"_n, wflbrnchmeta> brnchmetas;

    //branches for levels
    TABLE wflbranch {
      uint64_t id;
      uint64_t idrootlvl = 0;
      uint64_t idparent = 0;
      uint64_t idmeta;
      name winner;
      uint64_t generation = 1;

      uint64_t primary_key() const { return id; }
      uint64_t get_idmeta() const { return idmeta; }
    };
    typedef multi_index<"branches"_n, wflbranch,
      indexed_by<"bymeta"_n, const_mem_fun<wflbranch, uint64_t, &wflbranch::get_idmeta>>
    > branches;

    //branch stakeholders with accumulated revenue and stake
    TABLE wflstake {
      uint64_t id;
      uint64_t idbranch;
      name owner;
      asset stake = asset{0, Const::acceptedSymbol};
      asset revenue = asset{0, Const::acceptedSymbol};

      uint64_t primary_key() const { return id; }
      uint64_t get_idbranch() const { return idbranch; }
      uint128_t get_ownedbrnch() const { return Utils::combineIds(owner.value, idbranch); }
    };
    typedef multi_index<"stakes"_n, wflstake,
      indexed_by<"bybranch"_n, const_mem_fun<wflstake, uint64_t, &wflstake::get_idbranch>>,
      indexed_by<"byownedbrnch"_n, const_mem_fun<wflstake, uint128_t, &wflstake::get_ownedbrnch>>
    > stakes;

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
      uint64_t get_idparent()const { return idparent; }
      uint64_t get_idbranch()const { return idbranch; }
    };
    typedef multi_index<"levels"_n, wfllevel,
      indexed_by<"byparent"_n, const_mem_fun<wfllevel, uint64_t, &wfllevel::get_idparent>>,//next level
      indexed_by<"bybranch"_n, const_mem_fun<wfllevel, uint64_t, &wfllevel::get_idbranch>>
    > levels;

    //branch quests
    TABLE wflquest {
      uint64_t id;      
      name owner;
      asset balance = asset{0, Const::acceptedSymbol};
      std::vector<uint64_t> hashes;
      asset minprice = asset{0, Const::acceptedSymbol};
      asset maxprice = asset{0, Const::acceptedSymbol};
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
    
    bool addBalance(name to, asset amount, name payer);
    void subBalance(name from, asset amount, name payer);
    bool clearAccount(name account, name scope);
    void upsertChannel(name owner);
    void addPot(name owner, uint64_t idlevel, asset pot);
    void checkBranchMetaUsage(uint64_t idmeta);
    void tryTurnChecks(const wflplayer& _player);
    void commitPlayersTurn(wflplayer& p, const wfllevel& l);

    template<class T>
    std::vector<T> generateCells(randomizer& rnd, T size, T maxval);
    uint64_t addLevel(name owner, const wflbranch& branch);
    void registerStake(name owner, uint64_t idbranch, asset amount);
};