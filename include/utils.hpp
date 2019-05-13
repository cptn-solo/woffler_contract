#pragma once
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/asset.hpp>

using namespace eosio;

namespace Utils {
  
  inline uint64_t nextPrimariKey(uint64_t key) {
    if(key > 0) {
      return key;
    } else {
      return 1;
    }
  }

  inline uint32_t now() {
    return time_point_sec(current_time_point()).utc_seconds;
  }

  inline uint128_t combineIds(const uint64_t& x, const uint64_t& y) {
    return (uint128_t{x} << 64) | y;
  }

  bool hasIntersection(uint16_t& v1, uint16_t& v2) {    
    return (v1&v2) > 0;
  }

}
