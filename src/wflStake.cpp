#include <woffler.hpp>

void woffler::stkaddval(name owner, uint64_t idbranch, 
  asset amount
) {
  require_auth(owner);
  
  registerStake(owner, idbranch, amount);

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

  //TODO: find stake and add amount, or emplace if not found

  auto idstake = Utils::nextPrimariKey(_stakes.available_primary_key());
  _stakes.emplace(owner, [&](auto& s) {
    s.id = idstake;
    s.idbranch = idbranch;
    s.owner = owner;
    s.stake += amount;
    s.revenue = asset{0, Const::acceptedSymbol};//only for emplace. modify should not change revenue!
  });

  print(" Branch stake registred, current stake owned: ", asset{amount});

}