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
  
  template<class T>
  void printVectorInt(const std::vector<T>& data) {
    for (typename std::vector<T>::const_iterator i = data.begin(); i != data.end(); ++i)
      eosio::print("[", std::to_string(*i), "]");
  }

}
