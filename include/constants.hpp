#pragma once
#include <eosio/symbol.hpp>

namespace Const {
  enum playerstate {
    INIT, SAFE, RED, GREEN, TAKE
  };
  const eosio::symbol_code acceptedCurr("EOS");
  const eosio::symbol acceptedSymbol(acceptedCurr, 4);
  const uint8_t maxLvlLength(64);//maximum number of cells per level to restrict resource consumption (and maybe implement binary search later)
  const uint8_t retriesCount(3);
  const uint8_t tryturnMaxDistance(12);
  const uint8_t houseShare(3);//% of each stake added to branch
  
}

