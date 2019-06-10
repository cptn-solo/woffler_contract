#pragma once
#include <eosio/symbol.hpp>

namespace Const {
  enum playerstate: uint8_t {
    INIT, SAFE, RED, GREEN, TAKE, NEXT, SPLIT
  };
  enum revenuetype: uint8_t {
    UNJAIL, BUYTRIES
  };
  const eosio::symbol_code acceptedCurr("EOS");
  const eosio::symbol acceptedSymbol(acceptedCurr, 4);
  const uint8_t lvlLength(16);//number of cells per level (uint16_t binary representation used as "level map")
  const uint8_t retriesCount(3);
  const uint8_t tryturnMaxDistance(12);
  const uint8_t houseShare(3);//% of each stake added to branch
  const uint8_t maxChannelRate(10);
  const uint64_t branchTipExpSeconds(2592000);//30days
}

