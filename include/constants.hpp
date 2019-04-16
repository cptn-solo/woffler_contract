#pragma once
#include <eosio/symbol.hpp>

enum playerstate {
    INIT, SAFE, RED, GREEN, TAKE
};
const eosio::symbol_code acceptedCurr("EOS");
const eosio::symbol acceptedSymbol(acceptedCurr, 4);

