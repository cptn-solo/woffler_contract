#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

namespace Utils {
  
  uint64_t nextPrimariKey(uint64_t key) {
    if(key > 0) {
      return key;
    } else {
      return 1;
    }
  }
}
