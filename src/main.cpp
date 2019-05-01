#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;
    
    CONTRACT woffler : public contract {
        public:
        using contract::contract;
        woffler(name receiver, name code, datastream<const char*> ds): 
            contract(receiver, code, ds) {}
            
        //signup new player with custom sales channel (via referral link)
        ACTION signup(name account, name channel) {
            Player::Player player = Player::Player(get_self(), account);
            player.createPlayer();
        }
    };
}