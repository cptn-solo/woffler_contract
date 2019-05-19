#include <woffler.hpp>
#include "wflPlayer.cpp"
#include "wflBranch.cpp"
#include "wflLevel.cpp"

//*** Contract scope methods ***//
namespace woffler {

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