//just random simpliest and shortest "random" generator
//https://eosio.stackexchange.com/questions/41/how-can-i-generate-random-numbers-inside-a-smart-contract
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>

using namespace eosio;

class randomizer {
private:
    static randomizer instance;

    uint64_t seed = 0;

public:
    static randomizer& getInstance(name player) {
        auto _now = time_point_sec(current_time_point()).utc_seconds;
        auto _player = player.value;

        instance.seed = _now + player.value;

        return instance;
    }

    uint32_t range(uint32_t to) {
        print("seed:", std::to_string(seed), "\n");
        checksum256 result = sha256((char *)&seed, sizeof(seed));
        result.print();
        print("\n");

        auto dgarr = result.get_array();
        seed = dgarr[1];
        seed <<= 32;
        seed |= dgarr[0];
        return (uint32_t)(seed % to);
    }
};