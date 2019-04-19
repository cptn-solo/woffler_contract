#include <woffler.hpp>

void woffler::branch(name owner, uint64_t idmeta, 
  asset pot
) {
  require_auth(owner);
  print("Creating branch with idmeta: ", std::to_string(idmeta));
  
  //create root branch with root level after meta is created/selected from existing
  auto self = get_self();
  
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(idmeta);
  check(
    _meta != _metas.end(),
    "No branch metadata found for id"
  );

  //cut owner's active balance for pot value (will fail if not enough funds)
  subBalance(owner, pot);

  branches _branches(self, self.value);
  auto idbranch = Utils::nextPrimariKey(_branches.available_primary_key());
  _branches.emplace(owner, [&](auto& b) {
    b.id = idbranch;
    b.idmeta = idmeta;
    b.idparent = 0;//root branch
    b.generation = 1;//root branch
  });

  //add pot value from owner's active balance to the root level's pot
  addLevel(owner, idbranch, pot, *_meta);
  
  //register pot value as owner's stake in root branch created
  registerStake(owner, idbranch, pot);

  print(" Root branch created, pot: ", asset{pot});
}

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