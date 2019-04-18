#include <woffler.hpp>
#include "accounting.cpp"

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
  levels _levels(self, self.value);
  auto idlevel = Utils::nextPrimariKey(_levels.available_primary_key());
  std::vector<uint8_t> redcells = {};//TODO: fill with random numbers (1-Const::levelLength)
  std::vector<uint8_t> greencells = {};//TODO: fill with random numbers (1-Const::levelLength)
  _levels.emplace(owner, [&](auto& l) {
    l.id = idlevel;
    l.idbranch = idbranch;
    l.idchbranch = 0;
    l.potbalance = pot;//owner's balance cut with check (see subBalance earlier)
    l.redcells = redcells;
    l.greencells = greencells;
  });
  
  //register pot value as owner's stake in root branch created
  stakes _stakes(self, self.value);
  auto idstake = Utils::nextPrimariKey(_stakes.available_primary_key());
  _stakes.emplace(owner, [&](auto& s) {
    s.id = idstake;
    s.idbranch = idbranch;
    s.owner = owner;
    s.stake = pot;
    s.revenue = asset{0, Const::acceptedSymbol};
  });
  
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