#include <woffler.hpp>

ACTION woffler::signup(name account, uint64_t idchannel) {
  require_auth(account);
  print("Register user: ", name{account});

  auto itr = _players.find(account.value);
  if (itr == _players.end()) {
      _players.emplace(get_self(), [&](auto& p) {
        p.account = account;
        p.levelresult = playerstate::INIT;
        //TODO: check existence of the channel and use 0 if no channel found
        p.idchannel = idchannel;
      });
    print(", added");
  } else {
    print(", exists");
  }
}

void woffler::deposit(name from, name to, asset quantity, string memo) {
  require_auth(from);
  print("deposting: ", asset{quantity}, " to account: ", name{from});
}

extern "C" { 
  void apply( uint64_t receiver, uint64_t code, uint64_t action ) { 
    if(code == receiver) {
        switch(action) {
            EOSIO_DISPATCH_HELPER(
                woffler,
                (signup)                    
            )
        }
    } else if(code == "eosio.token"_n.value && action == "transfer"_n.value) {
      execute_action<woffler>(name(receiver), name(code), &woffler::deposit);
    }
  } 
}