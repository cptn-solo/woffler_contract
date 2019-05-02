#include <utils.hpp>
#include <constants.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;
    
    namespace Player {
        class Player {
            public:
            Player(name self, name account);
            //players with there balances and in-game state
            TABLE wflplayer {
                name account;
                name channel;
                uint64_t idlvl = 0;
                asset activebalance = asset{0, Const::acceptedSymbol};
                asset vestingbalance = asset{0, Const::acceptedSymbol};
                uint8_t tryposition = 0;
                uint8_t currentposition = 0;
                uint8_t triesleft = 0;
                uint8_t levelresult = Const::playerstate::INIT;
                uint32_t resulttimestamp;
                
                uint64_t primary_key() const { return account.value; }
            };
            typedef multi_index<"players"_n, wflplayer> players;    

            const wflplayer& getPlayer();
            
            void checkNoPlayer();
            
            void createPlayer(name payer, name channel);            

            private:
            name _self;
            name _player;
            players::const_iterator _pitr;
            
            players _players;     

        };
    }
}
