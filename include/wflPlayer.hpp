#pragma once
#include <utils.hpp>
#include <constants.hpp>

using namespace eosio;
using std::string;

class Player {
    public:
    Player(name self, name player);

    bool isRegistred();

    void checkPlayer();
    void checkNoPlayer();
    void checkActivePlayer();
    void checkState(Const::playerstate state);
    void checkBalanceCovers(asset amount);
    void checkBalanceZero();
    void checkSwitchBranchAllowed();
    void checkLevelUnlockTrialAllowed(uint64_t idlvl);

    void createPlayer(name channel, name payer);
    void addBalance(asset amount, name payer);
    void subBalance(asset amount, name payer);
    void switchRootLevel(uint64_t idlvl);
    void useTry();
    void useTry(uint8_t position);
    void commitTurn(Const::playerstate result);
    void resetPositionAtLevel(uint64_t idlvl);

    void rmAccount();        
    
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
        EOSLIB_SERIALIZE(wflplayer, (account)(channel)(idlvl)(activebalance)(vestingbalance)(tryposition)(currentposition)(triesleft)(levelresult)(resulttimestamp))
    };
    typedef multi_index<"players"_n, wflplayer> players;        

    players::const_iterator* player = NULL;

    private:
    
    template<typename Lambda>
    void updateState(name payer, Lambda&& updater);        
    
    name _self;
    name _player;
    players _players;            
};