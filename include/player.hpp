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
      uint64_t idlevel = 0;
      asset activebalance = asset{0, Const::acceptedSymbol};
      asset vestingbalance = asset{0, Const::acceptedSymbol};
      uint8_t tryposition = 0;
      uint8_t currentposition = 0;
      uint8_t triesleft = 0;
      uint8_t status = Const::playerstate::INIT;
      uint32_t resulttimestamp;

      uint64_t primary_key() const { return account.value; }
      uint64_t get_channel() const { return channel.value; }
      uint64_t get_idlevel() const { return idlevel; }
    } wflplayer;

    typedef multi_index<"players"_n, wflplayer,
      indexed_by<"bychannel"_n, const_mem_fun<wflplayer, uint64_t, &wflplayer::get_channel>>,
      indexed_by<"byidlevel"_n, const_mem_fun<wflplayer, uint64_t, &wflplayer::get_idlevel>>
    > players;

    class DAO: public Accessor<players, wflplayer, players::const_iterator, uint64_t>  {
      public:
      DAO(players& _players, uint64_t _playerV):
      Accessor<players, wflplayer, players::const_iterator, uint64_t>::Accessor(_players, _playerV) {}

      DAO(players& _players, players::const_iterator itr):
      Accessor<players, wflplayer, players::const_iterator, uint64_t>::Accessor(_players, itr) {}

      static uint64_t keyValue(name account) {
        return account.value;
      }
    };

    class Player: Entity<players, DAO, name, wflplayer> {
      private:
      wflplayer _player;

      public:
      Player(name self, name account) : Entity<players, DAO, name, wflplayer>(self, account) {
        if (isEnt())
          _player = getPlayer();
      }

      wflplayer getPlayer() {
        return getEnt();
      }
      
      name getChannel() {
        return getPlayer().channel;
      }

      bool isPlayer();//true if player exists in registry
      void checkReferrer(name referrer);//referrer exists in registry
      void checkNotReferrer();//player is not a referrer
      void checkPlayer();//player registred
      void checkNoPlayer();//player NOT registred
      void checkActivePlayer();//player is positioned in branch (playing)
      void checkState(Const::playerstate state);//player is in specified state
      void checkBalanceCovers(asset amount);//player's active balance is not less then specified
      void checkBalanceZero();//player's active balance is zero
      void checkLevelUnlockTrialAllowed();//player can proceed with specified level unlocking trial

      void createPlayer(name payer, name referrer);
      void addBalance(asset amount, name payer);
      void subBalance(asset amount, name payer);
      void claimVesting();
      void clearVesting();
      void switchBranch(uint64_t idbranch);
      void switchRootLevel(uint64_t idlevel, Const::playerstate playerState);
        
      void useTry();
      void useTry(uint8_t position);
      void commitTake(asset amount, uint32_t timestamp);
      void commitTurn(Const::playerstate status);

      void resetPositionAtLevel(uint64_t idlevel);
      void resetRetriesCount();

      void rmAccount();

      //DEBUG:
      void reposition(uint64_t idlevel, uint8_t position);
      void rmPlayer();
    };
  }
}
