#include "accounting.cpp"
#include "wflChannel.cpp"
#include "wflBranch.cpp"

//*** Contract scope methods ***//

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
  upsertChannel(_channel);
}

void woffler::forget(name account) {
  require_auth(account);
  print("Forget user: ", name{account});

  clearAccount(account, get_self());
  clearAccount(account, name("eosio.token"));//legacy
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