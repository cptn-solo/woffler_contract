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

  auto minPot = (((_meta->stkmin * 100) / _meta->stkrate) * 100) / _meta->spltrate;
  check(
    /*
    pot - value to be placed to the root level upon creation; must be covered by creator's active balance;
    spltrate - % of level's current pot moved to new branch upon split action from winner;
    stkrate - % of level's current pot need to be staked by each pretender to be a stakeholder of the branch upon split;
    stkmin - minimum value accepted as "stake" for the branch;

    pot * spltrate% * stkrate% > stkmin
    (((pot * spltrate)/100) * stkrate)/100 > stkmin
    (((pot * spltrate)/100) * stkrate) > stkmin * 100
    ((pot * spltrate)/100) > (stkmin * 100) / stkrate
    pot > (((stkmin * 100) / stkrate) * 100) / spltrate

    for stkmin = 10, stkrate = 3, spltrate = 50 we'll get minimum pot value as
    (((10*100)/3)*100)/50 = 666

    for stkmin = 1, stkrate = 10, spltrate = 50 we'll get minimum pot value as
    (((1*100)/10)*100)/50 = 20
    
    */
    minPot <= pot,    
    string("Branch minimum pot is ")+minPot.to_string().c_str()
  );
  
  //cut owner's active balance for pot value (will fail if not enough funds)
  subBalance(owner, pot, owner);

  branches _branches(self, self.value);
  auto idbranch = Utils::nextPrimariKey(_branches.available_primary_key());
  _branches.emplace(owner, [&](auto& b) {
    b.id = idbranch;
    b.idmeta = idmeta;
  });
  
  //register players's and house stake
  auto playerStake = (pot * (100 - Const::houseShare)) / 100;
  registerStake(owner, idbranch, playerStake);

  auto houseStake = (pot * Const::houseShare) / 100;
  registerStake(self, idbranch, houseStake);

  print(" Root branch created, pot: ", asset{pot});
}

void woffler::rootlvl(name owner, uint64_t idbranch) {
  require_auth(owner);
  print("Creating root level for branch with id: ", std::to_string(idbranch));
  
  auto self = get_self();
  
  branches _branches(self, self.value);
  auto _branch = _branches.find(idbranch);
  check(
    _branch != _branches.end(),
    "No branch found for id"
  );
  check(
    _branch->idrootlvl == 0,
    "Root level already exists"
  );

  //find stake to use as pot value for root level
  stakes _stakes(self, self.value);

  auto ownedBranchId = Utils::combineIds(owner.value, idbranch);    
  auto sktidx = _stakes.get_index<name("byownedbrnch")>();
  auto stake = sktidx.find(ownedBranchId);          

  check(
    stake != sktidx.end(),
    "Only stakeholder of a branch can create root level for it"
  );
  
  //add pot value from owner's active balance to the root level's pot
  uint64_t idrootlvl = addLevel(owner, *_branch);
  _branches.modify(_branch, owner, [&](auto& b){
    b.idrootlvl = idrootlvl;
  });
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

//DEBUG: update idrootlvl
void woffler::setrootlvl(name owner, uint64_t idbranch, uint64_t idrootlvl) {
  require_auth(owner);
  auto self = get_self();
  check(
    owner == self,
    string("Debug mode available only to contract owner: ") + self.to_string()
  );

  branches _branches(self, self.value);
  auto _branch = _branches.find(idbranch);
  check(
    _branch != _branches.end(),
    "No branch found for id"
  );

  _branches.modify(_branch, owner, [&](auto& b){
    b.idrootlvl = idrootlvl;
  });
}
