#pragma once
#include <utils.hpp>
#include <constants.hpp>

namespace woffler {
  using namespace eosio;
  using std::string;

  CONTRACT woffler : public contract {
    public:
      using contract::contract;
      woffler(name receiver, name code, datastream<const char*> ds): 
        contract(receiver, code, ds) {}
            
      #pragma region ** Player (wflPlayer): **

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

      void upsertChannel(name owner);
      void checkBranchMetaUsage(uint64_t idmeta);

      uint64_t addLevel(name owner, uint64_t idbranch, uint64_t idmeta);
      void registerStake(name owner, uint64_t idbranch, asset amount);
  };
}