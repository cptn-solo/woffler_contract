#pragma once
#include <utils.hpp>
#include <constants.hpp>

namespace woffler {
    using namespace eosio;
    using std::string;

    namespace wflPlayer {
        class Player {
            public:
            Player(name self, name player);

            uint64_t idlevel = 0;
            uint8_t levelresult = Const::playerstate::INIT;
            uint8_t tryposition = 0;
            uint8_t currentposition = 0;
            uint8_t triesleft = 0;
            asset activebalance = asset{0, Const::acceptedSymbol};
            name channel = name();

            void checkPlayer();
            void checkNoPlayer();
            void checkActivePlayer();
            void checkState(Const::playerstate state);
            void checkBalanceCovers(asset amount);
            void checkBalanceZero();
            void checkSwitchBranchAllowed();
            void checkLevelUnlockTrialAllowed(uint64_t idlvl);

            void createPlayer(name achannel);
            bool addBalance(asset amount, name payer);
            void subBalance(asset amount, name payer);
            void switchRootLevel(uint64_t idlvl);
            void useTry();
            void useTry(uint8_t position);
            void commitTurn(Const::playerstate result);
            void resetPositionAtLevel(uint64_t idlvl);

            bool rmAccount();        
            
            private:
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
            
            template<typename Lambda>
            void updateState(name payer, Lambda&& updater);        
            
            name _self;
            name _player;
            players _players;            
        };
    }

}
