#include <woffler.hpp>

void woffler::signup(name account, uint64_t idchannel) {
  require_auth(account);
  print("Register user: ", name{account});

  auto self = get_self();

  playerstable _players(self, self.value);
  
  auto player = _players.find(account.value);
  
  check(
    player == _players.end(), 
    "Account already exists"
  );
  
  _players.emplace(self, [&](auto& p) {
    p.account = account;
    p.levelresult = playerstate::INIT;
    //TODO: check existence of the channel and use 0 if no channel found
    p.idchannel = idchannel;
    p.activebalance = asset{0, acceptedSymbol};
    p.vestingbalance = asset{0, acceptedSymbol};
  });
}

void woffler::deposit(name from, name to, asset amnt, string memo) {
  print("deposting: ", asset{amnt}, " to account: ", name{from});

  check(
    amnt.symbol.code() == acceptedCurr,
    "Deposits accepted only in " + acceptedCurr.to_string() + " tokens"
  );
  
  check(
    to == get_self(), 
    "Contract must be a receiver"
  );
  
  bool deposited = woffler::appendBalance(from, amnt);

  if (!deposited) {
    signup(from, 0);
    appendBalance(from, amnt);
  }
}

bool woffler::appendBalance(name from, asset amnt) {
  auto self = get_self();

  playerstable _players(self, self.value);
  
  auto player = _players.find(from.value);
  
  if (player == _players.end()) 
    return false;
    
  _players.modify(player, self, [&]( auto& p ) {
    p.activebalance += amnt;     
  });
  
  print("Current balance: ", asset{player->activebalance});
  
  return true;  
}