//just random simpliest and shortest "random" generator
//https://eosio.stackexchange.com/questions/41/how-can-i-generate-random-numbers-inside-a-smart-contract
#include <utils.hpp>

class randomizer {
  private:
    static randomizer instance;

    uint64_t seed = 0;

  public:
    static randomizer& getInstance(name player, uint64_t addint) {
        auto _now = Utils::now();
        auto _player = player.value;

        instance.seed = _now + player.value + addint;

        return instance;
    }

    uint32_t range(uint32_t to) {
        print("seed:", std::to_string(seed), "\n");
        checksum256 result = sha256((char *)&seed, sizeof(seed));

        auto dgarr = result.get_array();
        seed = dgarr[1];
        seed <<= 32;
        seed |= dgarr[0];
        return (uint32_t)(seed % to);
    }
};  

