#pragma once
#include <eosio/symbol.hpp>

namespace Const {
  enum playerstate {
    INIT, SAFE, RED, GREEN, TAKE
  };
  const eosio::symbol_code acceptedCurr("EOS");
  const eosio::symbol acceptedSymbol(acceptedCurr, 4);
  const uint8_t houseShare(3);//% of each stake added to branch
}

