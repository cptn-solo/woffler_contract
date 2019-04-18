#pragma once
#include <eosio/symbol.hpp>

namespace Const {
  enum playerstate {
    INIT, SAFE, RED, GREEN, TAKE
  };
  const uint8_t levelLength(16);
  const eosio::symbol_code acceptedCurr("EOS");
  const eosio::symbol acceptedSymbol(acceptedCurr, 4);
}

