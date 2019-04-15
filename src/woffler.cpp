#include <woffler.hpp>

ACTION beltalpha21z::signup(name account, uint64_t idchannel) {
  require_auth(account);
  print("Register user: ", name{account});

  auto itr = _players.find(account.value);
  check(itr == _players.end(), "Account already exists");

  _players.emplace(get_self(), [&](auto& p) {
    p.account = account;
    p.levelresult = playerstate::INIT;
    //TODO: check existence of the channel and use 0 if no channel found
    p.idchannel = idchannel;
    p.activebalance = asset{0, acceptedSymbol};
    p.vestingbalance = asset{0, acceptedSymbol};
  });
}

void beltalpha21z::deposit(name from, name to, asset amnt, string memo) {
  print("deposting: ", asset{amnt}, " to account: ", name{from});

  check(
    amnt.symbol.code() == acceptedCurr,
    "Deposits accepted only in " + acceptedCurr.to_string() + " tokens"
  );
  check(to == get_self(), "Contract must be a receiver");
  
  bool deposited = beltalpha21z::appendBalance(from, amnt);
  if (!deposited) {
    beltalpha21z::signup(from, 0);
    beltalpha21z::appendBalance(from, amnt);
  }
}

bool beltalpha21z::appendBalance(name from, asset amnt) {
  auto itr = _players.find(from.value);
  if (itr != _players.end()) {
    _players.modify(itr, get_self(), [&]( auto& p ) {
      p.activebalance += amnt;     
      p.triesleft = 3;   
    });
    print("Current balance: ", asset{itr->activebalance});
    return true;
  }
  return false;
}

extern "C" { 
  void apply( uint64_t receiver, uint64_t code, uint64_t action ) { 
    if(code == receiver) {
        switch(action) {
            EOSIO_DISPATCH_HELPER(
                beltalpha21z,
                (signup)(deposit)                    
            )
        }
    } else if(code == "eosio.token"_n.value && action == "transfer"_n.value) {
      execute_action<beltalpha21z>(name(receiver), name(code), &beltalpha21z::deposit);
    }
  } 
}