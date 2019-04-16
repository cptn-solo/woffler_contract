#include <woffler.hpp>

//*** Contract scope methods ***//

//register new player and assing to (existing) sales channel or contract channel
void woffler::signup(name account, name channel) {
  require_auth(account);
  print("Register user: ", name{account});

  auto self = get_self();

  auto _channel = (channel ? channel : self);
  
  players _players(self, self.value);
  
  auto player = _players.find(account.value);
  
  check(
    player == _players.end(), 
    "Account already exists"
  );

  //validating sales channel
  if (_channel != self) { //channel account must be signed up already
    players _chnlaccts(self, self.value);
    
    auto chnlacct = _chnlaccts.find(channel.value);

    check(
      chnlacct != _players.end(),
      "Channel account does not exits"
    );
  }
  
  //creating player record
  _players.emplace(self, [&](auto& p) {
    p.account = account;
    p.levelresult = playerstate::INIT;
    //TODO: check existence of the channel and use 0 if no channel found
    p.channel = _channel;
    p.activebalance = asset{0, acceptedSymbol};
    p.vestingbalance = asset{0, acceptedSymbol};
  });
  
  //creating/incrementing sales channel
  channels _channels(self, self.value);
  auto achannel = _channels.find(_channel.value);

  if (achannel == _channels.end()) {
    _channels.emplace(self, [&](auto& c) {
      c.owner = _channel;
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
// remove account from players table from contract (and eosio.token, legacy) scope
void woffler::forget(name account) {
  require_auth(account);
  print("Forget user: ", name{account});

  clearAccount(account, get_self());
  clearAccount(account, name("eosio.token"));//legacy
}

bool woffler::clearAccount(name account, name scope) {
  auto self = get_self();

  players _players(self, scope.value);
  
  auto player = _players.find(account.value);
  
  if (player == _players.end()) 
    return false;

  _players.erase(player);
  
  print("Removed user: ", name{account}, " from scope: ", name{});
  
  return true;
}

void woffler::transferHandler(name from, name to, asset amount, string memo) {
  print("Transfer: ", asset{amount}, " from: ", name{from}, " to: ", name{to});

  check(
    amount.symbol.code() == acceptedCurr,
    "Only " + acceptedCurr.to_string() + " transfers allowed"
  );

  auto self = get_self();

  if (to == self) { //deposit
    bool deposited = woffler::addBalance(from, amount);

    if (!deposited) {
      signup(from, self);
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

  players _players(self, self.value);
  
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

  players _players(self, self.value);
  
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

//*** Sales channel scope methods (wflChannel) ***//

void woffler::chnmergebal(name owner) {

}