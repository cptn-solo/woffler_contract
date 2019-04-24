#include <woffler.hpp>

void woffler::stkaddval(name owner, uint64_t idbranch, 
  asset amount
) {
  require_auth(owner);
  
  auto self = get_self();
  branches _branches(self, self.value);
  auto _branch = _branches.find(idbranch);
  check(
    _branch != _branches.end(),
    "No branch found for id"
  );

  //cut owner's active balance for pot value (will fail if not enough funds)
  subBalance(owner, amount, owner);
  
  if (_branch->generation > 1) {
    registerStake(owner, idbranch, amount);
  } 
  else {
    //register players's and house stake
    auto playerStake = (amount * (100 - Const::houseShare)) / 100;
    registerStake(owner, idbranch, playerStake);

    auto houseStake = (amount * Const::houseShare) / 100;
    registerStake(self, idbranch, houseStake);
  }

  //if root level is created already - append staked value to the root level's pot
  if(_branch->idrootlvl > 0) {
    addPot(owner, _branch->idrootlvl, amount);
  }
}

void woffler::stktakervn(name owner, uint64_t idbranch) {
  require_auth(owner);
  check(false, "Not implemented");
}

void woffler::registerStake(name owner, uint64_t idbranch, 
  asset amount
) {
  auto self = get_self();
  stakes _stakes(self, self.value);

  //find stake and add amount, or emplace if not found
  auto ownedBranchId = Utils::combineIds(owner.value, idbranch);    
  auto stkidx = _stakes.get_index<name("byownedbrnch")>();
  const auto& stake = stkidx.find(ownedBranchId);          

  auto currentStake = amount;        

  if (stake == stkidx.end()) {
    auto idstake = Utils::nextPrimariKey(_stakes.available_primary_key());
    _stakes.emplace(owner, [&](auto& s) {
      s.id = idstake;
      s.idbranch = idbranch;
      s.owner = owner;
      s.stake = amount;
      s.revenue = asset{0, Const::acceptedSymbol};//only for emplace. modify should not change revenue!
    });
  } 
  else {
    _stakes.modify(*stake, owner, [&](auto& s) {
      s.stake += amount;     
    });    
    currentStake = stake->stake;
  }

  print(" Branch stake registred, current stake owned: ", asset{currentStake});
}