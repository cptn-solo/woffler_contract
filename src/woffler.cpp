#include <woffler.hpp>

ACTION woffler::signup(name account) {
  require_auth(account);
  print("Register user: ", name{account});

  auto itr = _players.find(account.value);
  if (itr == _players.end()) {
      _players.emplace(get_self(), [&](auto& p) {
        p.account = account;
        p.levelresult = playerstate::INIT;
      });
    print(", added");
  } else {
    print(", exists");
  }
}
