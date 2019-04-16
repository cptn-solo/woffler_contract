#include <woffler.hpp>
#include "accounting.cpp"

//*** Sales channel scope methods (wflChannel) ***//

void woffler::upsertChannel(name owner) {
  auto self = get_self();

  channels _channels(self, self.value);
  auto achannel = _channels.find(owner.value);

  if (achannel == _channels.end()) {
    _channels.emplace(self, [&](auto& c) {
      c.owner = owner;
      c.height = 1;
      c.balance = asset{0, acceptedSymbol};
    });
  } 
  else {
    _channels.modify(achannel, self, [&]( auto& p ) {
      p.height++;     
    });
  }
}

void woffler::chnmergebal(name owner) {
  auto self = get_self();
  require_auth(owner);
  
  channels _channels(self, self.value);
  auto achannel = _channels.find(owner.value);

  check(
    achannel != _channels.end(),
    "No channel found"
  );
  
  auto amount = achannel->balance;

  _channels.modify(achannel, self, [&]( auto& c ) {
    c.balance = asset{0, acceptedSymbol};     
  });
  
  addBalance(owner, amount);
  
  print("Channel balance merged: ", asset{amount});
}