#pragma once
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/print.hpp>

namespace Utils {
  
  inline uint64_t nextPrimariKey(uint64_t key) {
    if(key > 0) {
      return key;
    } else {
      return 1;
    }
  }
  
  template<typename T>
  void printVectorInt(const std::vector<T>& data) {
    for (typename std::vector<T>::const_iterator i = data.begin(); i != data.end(); ++i)
      eosio::print("[", std::to_string(*i), "]");
  }
  inline uint128_t combineIds(const uint64_t& x, const uint64_t& y) {
    return (uint128_t{x} << 64) | y;
  }

  template<typename T>
  bool hasIntersection(std::vector<T>& v1, std::vector<T>& v2) {
    typename std::vector<T> v3;
    
    //our vectors are sorted by design
    // std::sort(v1.begin(), v1.end());
    // std::sort(v2.begin(), v2.end());

    std::set_intersection(v1.begin(), v1.end(),
                          v2.begin(), v2.end(),
                          back_inserter(v3));
    return !v3.empty();
  }

}
