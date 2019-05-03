#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>
#include <channel.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;
    class
    [[eosio::contract("woffler")]]
    woffler : public contract {
        public:
        using contract::contract;
        woffler(name receiver, name code, datastream<const char*> ds): 
            contract(receiver, code, ds) {}
            
        //signup new player with custom sales channel (via referral link)
        ACTION signup(name account, name referrer) {
            require_auth(account);

            auto self = get_self();
            auto _referrer = (referrer ? referrer : _self);

            Player::Player player = Player::Player(self, account);
            player.createPlayer(account, _referrer);//player pays RAM to store his record

            Channel::Channel channel = Channel::Channel(self, _referrer);            
            channel.upsertChannel(self);//contract pays RAM for the sales channels' record
        }
    };
}