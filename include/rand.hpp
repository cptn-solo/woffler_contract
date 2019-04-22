//just random simpliest and shortest "random" generator
//https://eosio.stackexchange.com/questions/41/how-can-i-generate-random-numbers-inside-a-smart-contract
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

class randomGen {
private:
    static randomGen instance;

    uint64_t seed = 0;

public:
    static randomGen& getInstance(name player) {
        if (instance.seed == 0) {
            instance.seed = current_time_point().sec_since_epoch() + player.value;
        }
        return instance;
    }

    uint32_t range(uint32_t to) {
        checksum256 result = sha256((char *)&seed, sizeof(seed));
        //sha256((char *)&seed, sizeof(seed), &result);
        auto dgarr = result.get_array();
        seed = dgarr[1];
        seed <<= 32;
        seed |= dgarr[0];
        return (uint32_t)(seed % to);
    }
};