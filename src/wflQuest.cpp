#include <woffler.hpp>

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