#pragma once
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
      uint64_t get_channel() const { return channel.value; }
    } wflplayer;

    typedef multi_index<"players"_n, wflplayer,
      indexed_by<"bychannel"_n, const_mem_fun<wflplayer, uint64_t, &wflplayer::get_channel>>
    > players;

    class DAO: public Accessor<players, wflplayer, players::const_iterator, uint64_t>  {
      public:
      DAO(players& _players, uint64_t _playerV);
      DAO(players& _players, players::const_iterator itr);
      static uint64_t keyValue(name account) {
        return account.value;
      }
    };

    class Player: Entity<players, DAO, name> {
      public:
      Player(name self, name account);

      wflplayer getPlayer();
      name getChannel();

      bool isPlayer();//true if player exists in registry
      void checkReferrer(name referrer);//referrer exists in registry
      void checkNotReferrer();//player is not a referrer
      void checkPlayer();//player registred
      void checkNoPlayer();//player NOT registred
      void checkActivePlayer();//player is positioned in branch (playing)
      void checkState(Const::playerstate state);//player is in specified state
      void checkBalanceCovers(asset amount);//player's active balance is not less then specified
      void checkBalanceZero();//player's active balance is zero
      void checkSwitchBranchAllowed();//player can change branch
      void checkLevelUnlockTrialAllowed(uint64_t idlvl);//player can proceed with specified level unlocking trial

      void createPlayer(name payer, name referrer);
      void addBalance(asset amount, name payer);
      void subBalance(asset amount, name payer);
      void switchBranch(uint64_t idbranch);
      void switchRootLevel(uint64_t idlvl);
      void tryTurn();
      void commitTurn();
      void useTry();
      void useTry(uint8_t position);
      void commitTurn(Const::playerstate result);

      void claimGreen();
      void claimRed();
      void resetPositionAtLevel(uint64_t idlvl);

      void rmAccount();
      
      //DEBUG:
      void reposition(uint64_t idlevel, uint8_t position);
    };
  }
}
