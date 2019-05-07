#include <utils.hpp>
#include <constants.hpp>
#include "player.cpp"
#include "channel.cpp"
#include "branch.cpp"
#include "level.cpp"
#include "branchmeta.cpp"
#include "stake.cpp"
#include "quest.cpp"
#include "branchquest.cpp"

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
    
    #pragma region ** Handlers: **
    
    [[eosio::on_notify("eosio.token::transfer")]]
    void transferHandler(name from, name to, asset amount, string memo) {
      auto self = get_self();
      print("Processing transfer from: ", name{from}, " to: ", name{to}, " amount : ", asset{amount}, "\n");
      check(
        amount.symbol.code() == Const::acceptedCurr,
        "Only " + Const::acceptedCurr.to_string() + " transfers allowed"
      );

      if (to == self) { //deposit
        Player::Player player(self, from);
        if (!player.isPlayer()) {
          player.createPlayer(self, self);

          //transfer always register a user under contract's sales channel
          Channel::Channel channel(self, self);
          channel.upsertChannel(self);//contract pays RAM for the sales channels' record
        }
        player.addBalance(amount, self);
      }
    }

    using transferAction = action_wrapper<"transfer"_n, &woffler::transferHandler>;
    
    #pragma endregion

    #pragma region ** Contract (on-off boarding and deposit/withdraw): **
    
    //signup new player with custom sales channel (via referral link)
    ACTION signup(name account, name referrer) {
      require_auth(account);

      auto self = get_self();
      auto _referrer = (referrer ? referrer : _self);

      Player::Player player(self, account);
      player.createPlayer(account, _referrer);//player pays RAM to store his record

      Channel::Channel channel(self, _referrer);
      channel.upsertChannel(self);//contract pays RAM for the sales channels' record
    }

    //withdraw player's funds to arbitrary account (need auth by player)
    ACTION withdraw (name from, name to, asset amount, const string& memo) {
      require_auth(from);

      auto self = get_self();
      Player::Player player(self, from);
      player.subBalance(amount, from);

      // Inline transfer
      const auto& contract = name("eosio.token");
      transferAction t_action(contract, {self, "active"_n});
      t_action.send(self, to, amount, memo);
    }

    //forget player (without balance check yet, TBD!)
    ACTION forget(name account) {
      require_auth(account);

      auto self = get_self();

      Player::Player player(self, account);
      auto referrer = player.getChannel();
      player.rmAccount();

      Channel::Channel channel(self, referrer);
      channel.subChannel(self);
    }
    
    #pragma endregion
    
    #pragma region ** Branch presets (wflBranchMeta): **

    //create meta for root branch(es) - active balance must cover at least.
    //owner pays for ram to avoid spamming via branch meta creation.
    //only owner can modify branch metadata.
    ACTION brnchmeta(name owner, BranchMeta::wflbrnchmeta meta) {
        require_auth(owner);

        BranchMeta::BranchMeta branchmeta(get_self(), owner, meta);
        branchmeta.upsertBranchMeta();
      }

    //remove branch meta owned
    ACTION cleanbrmeta(name owner, uint64_t idmeta) {
      require_auth(owner);

      BranchMeta::BranchMeta branchmeta(get_self(), owner, idmeta);
      branchmeta.removeBranchMeta();
    }

    #pragma endregion

    #pragma region ** wflBranch**
    //create root branch after meta is created/selected from existing
    //register pot value as owner's stake in root branch created
    ACTION branch(name owner, uint64_t idmeta, asset pot) {
      require_auth(owner);

      Branch::Branch branch(get_self(), 0);
      branch.checkBranch();
    }
    #pragma endregion

    //set current root branch for player and position at 1st level
    ACTION switchbrnch(name account, uint64_t idbranch) {
      require_auth(account);

      auto self = get_self();

      Player::Player player(self, account);
      player.checkSwitchBranchAllowed();

      //find branch of the level
      Branch::Branch branch(self, idbranch);
      branch.checkStartBranch();

      uint64_t idrootlvl = branch.getRootLevel();

      //check if branch is unlocked (its root level is not locked)
      Level::Level level(self, idrootlvl);
      level.checkUnlockedLevel();

      //position player in root level of the branch
      player.switchRootLevel(idrootlvl);
    }
  };
}