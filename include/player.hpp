#include <entity.hpp>

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
        
        class DAO: Accessor<players, wflplayer, players::const_iterator, uint64_t>  {
            DAO(players& _players, uint64_t _playerV);
            static uint64_t keyValue(name account) {
                return account.value;
            }
        };

        class Player: Entity<players, DAO, name> {
            public:
            Player(name self, name account);
            
            name getChannel();
            
            bool isPlayer();
            void checkReferrer(name referrer);
            void checkPlayer();
            void checkNoPlayer();
            void checkActivePlayer();
            void checkState(Const::playerstate state);
            void checkBalanceCovers(asset amount);
            void checkBalanceZero();
            void checkSwitchBranchAllowed();
            void checkLevelUnlockTrialAllowed(uint64_t idlvl);

            void createPlayer(name payer, name referrer);                            
            void addBalance(asset amount, name payer);
            void subBalance(asset amount, name payer);
            void switchRootLevel(uint64_t idlvl);
            void useTry();
            void useTry(uint8_t position);
            void commitTurn(Const::playerstate result);
            void resetPositionAtLevel(uint64_t idlvl);

            void rmAccount();                        
        };

    }
}
