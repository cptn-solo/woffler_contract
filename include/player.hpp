#include <utils.hpp>
#include <constants.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;
    
    namespace Player {
        //players with there balances and in-game state
        typedef struct
        [[eosio::table("players"), eosio::contract("woffler")]]
        wflplayer {
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
        } wflplayer;
        
        typedef multi_index<"players"_n, wflplayer> players;    

        
        class Player {
            public:

                Player(name self, name account);

                void createPlayer(name payer, name channel);            

            private:

                name _self;
                name _player;            
                players _players;     

        };

        struct DAO {
            public:

                DAO(players& players, name player);      

                template<typename Lambda>
                void create(name payer, Lambda&& creator);

                template<typename Lambda>
                void update(name payer, Lambda&& updater);
                
                const wflplayer& getPlayer();
                void checkNoPlayer();
                void create(name payer, name player, name channel);
            
            private:

                players& _players;     
                players::const_iterator _pitr;
        };

    }
}