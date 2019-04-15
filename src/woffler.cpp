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

void woffler::transferHandler(name from, name to, asset amount, string memo) {
  print("Transfer: ", asset{amount}, " from: ", name{from}, " to: ", name{to});

  check(
    amount.symbol.code() == acceptedCurr,
    "Only " + acceptedCurr.to_string() + " transfers allowed"
  );

  if (to == get_self()) { //deposit
    bool deposited = woffler::addBalance(from, amount);

    if (!deposited) {
      signup(from, 0);
      addBalance(from, amount);
    }
  } 
    
}

void woffler::withdraw (name from, name to, asset amount, const string& memo) {
  auto self = get_self();
  require_auth(from);
  
  subBalance(from, amount);

  // Inline transfer
  const auto& contract = name("eosio.token");
  woffler::transferAction t_action(contract, {self, "active"_n});
  t_action.send(self, to, amount, memo);
   
  print("Withdrawn from: ", name{from}, " To: ", name{to});
}

bool woffler::addBalance(name to, asset amount) {
  auto self = get_self();

  playerstable _players(self, self.value);
  
  const auto& player = _players.find(to.value);
  
  if (player == _players.end()) 
    return false;
    
  _players.modify(player, self, [&]( auto& p ) {
    p.activebalance += amount;     
  });
  
  print("Current balance: ", asset{player->activebalance});
  
  return true;  
}

void woffler::subBalance(name from, asset amount) {
  auto self = get_self();

  playerstable _players(self, self.value);
  
  const auto& player = _players.find(from.value);
  
  check(
    player != _players.end(), 
    "Account does not exist"
  );
  
  check(
    player->activebalance >= amount, 
    string("Not enough active balance in your account. Current active balance: ") + player->activebalance.to_string().c_str() 
  );    

  _players.modify(player, self, [&]( auto& p ) {
    p.activebalance -= amount;     
  });     
}