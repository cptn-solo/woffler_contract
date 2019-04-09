#include <woffler.hpp>

ACTION woffler::signup(name account) {
  require_auth(account);
  print("Register user: ", name{account});
  _players.emplace(get_self(), [&](auto& p) {
        p.account = account;
        p.levelresult = playerstate::INIT;
    });
}
