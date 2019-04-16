#pragma once
#include <woffler.hpp>

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
