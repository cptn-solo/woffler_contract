#include <woffler.hpp>
#include "wflPlayer.cpp"
#include "wflBranch.cpp"
#include "wflLevel.cpp"

//*** Contract scope methods ***//
namespace woffler {

  #pragma region ** wflPlayer**

  void woffler::claimtake(name player) {

  }
  
  #pragma endregion

  #pragma region ** wflBranch**

  void woffler::addquest(name owner, uint64_t idbranch, 
    uint64_t idquest
  ) {
    require_auth(owner);
    check(false, "Not implemented");

  }

  void woffler::rmquest(name owner, uint64_t idbranch, 
    uint64_t idquest
  ) {
    require_auth(owner);
    check(false, "Not implemented");

  }

  #pragma endregion   
  
  #pragma region wflLevel

  void woffler::nextlvl(name player) {
    require_auth(player);
  }

  void woffler::takelvl(name player) {
    require_auth(player);
    //dont forget to set retries count = 0 to force a player to call `splitbet` before split branch unlock trial
  }

  void woffler::splitlvl(name player) {
    require_auth(player);  
  }

  void woffler::splitbet(name player) {
    require_auth(player);
    //players in TAKE state 1st cut vested balance, then - active
    //reset retries count if balance cut was successfull
  }

  #pragma endregion

  #pragma region wflChannel
  
  //*** Sales channel scope methods (wflChannel) ***//
  
  #pragma endregion

  #pragma region wflQuest
  
  void woffler::qstsetmeta(name owner, uint64_t idquest, 
    std::vector<uint64_t> hashes, 
    asset minprice, 
    asset maxprice, 
    string apiurl
  ) {
    require_auth(owner);
    check(false, "Not implemented");
      
  }

  void woffler::qstsetbal(name owner, uint64_t idquest, 
    asset amount
  ) {
    require_auth(owner);
    check(false, "Not implemented");

  }
  
  #pragma endregion

  #pragma region wflStake  

  void woffler::stktakervn(name owner, uint64_t idbranch) {
    require_auth(owner);
    check(false, "Not implemented");
  }


  
  #pragma endregion
}