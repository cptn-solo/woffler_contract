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
            
      #pragma region ** Branches (wflBranch): **

      //link quest created earlier to the specified branch (see qstsetmeta)
      ACTION addquest(name owner, uint64_t idbranch, uint64_t idquest);
      
      //unlink quest from the specified branch
      ACTION rmquest(name owner, uint64_t idbranch, uint64_t idquest);
      
      #pragma endregion

      #pragma region ** Branch Stakes (wflStake): **
        
      //merge branch stake revenue into owner's active balance
      ACTION stktakervn(name owner, uint64_t idbranch);

      #pragma endregion

      #pragma region ** Levels (wflLevel): **
      
      //make subbranch with locked root level
      //split level's pot according to level's branch metadata (`spltrate`, `potmin`)
      //make the player a stakeholder of new subbranch, share is defined by level's branch metadata (`stkrate`, `stkmin`)
      //as new level is locked, splitter have 3 tries to unlock it, if no luck - zero-ed in current level
      ACTION splitlvl(name player);

      //if no free unlock retries left, player can bet for split from his active balance to reset retries count  
      //bet amount is calculated according to level's branch metadata (`stkrate`, `stkmin`)
      ACTION splitbet(name player);

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
  };
}