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
        }
        player.addBalance(amount, self);
      }
    }

    using transferAction = action_wrapper<"transfer"_n, &woffler::transferHandler>;
    
    #pragma endregion

    #pragma region ** Contract (on-off boarding and deposit/withdraw): **

    void checkAdmin(name account) {
      require_auth(account);
      auto self = get_self();
      check(
        account == self,
        string("Debug mode available only to contract owner: ") + self.to_string()
      );      
    }
    
    //signup new player with custom sales channel (via referral link)
    ACTION signup(name account, name referrer) {
      require_auth(account);

      Player::Player player(get_self(), account);
      player.createPlayer(account, referrer);//player pays RAM to store his record
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

      Player::Player player(get_self(), account);
      player.rmAccount();
    }
    
    #pragma endregion
    
    #pragma region ** wflBranchMeta **

    //create meta for root branch(es) - active balance must cover at least.
    //owner pays for ram to avoid spamming via branch meta creation.
    //only owner can modify branch metadata.
    ACTION brnchmeta(name owner, BranchMeta::wflbrnchmeta meta) {
        require_auth(owner);

        BranchMeta::BranchMeta branchmeta(get_self(), meta.id);
        branchmeta.upsertBranchMeta(owner, meta);
      }

    //remove branch meta owned
    ACTION rmbrmeta(name owner, uint64_t idmeta) {
      require_auth(owner);

      BranchMeta::BranchMeta branchmeta(get_self(), idmeta);
      branchmeta.removeBranchMeta(owner);
    }

    #pragma endregion

    #pragma region ** wflBranch**
    
    //create root branch after meta is created/selected from existing
    //register pot value as owner's stake in root branch created
    ACTION branch(name owner, uint64_t idmeta, asset pot) {
      require_auth(owner);
      
      Branch::Branch branch(get_self(), 0);
      branch.createBranch(owner, idmeta, pot);      
    }
    
    //create root level with all branch stake (from all owners)
    //generate cells for root level
    ACTION rootlvl(name owner, uint64_t idbranch) {
      require_auth(owner);
      
      Branch::Branch branch(get_self(), idbranch);
      branch.createRootLevel(owner);
    }
    
    //revenue share, called as deferred action
    ACTION tipbranch(uint64_t idbranch, asset amount) {
      require_auth(get_self());
      
      print("Tipping branch with id <", std::to_string(idbranch), "> with amount: ", asset{amount}, ".\n");

      Branch::Branch branch(get_self(), idbranch);
      branch.allocateRevshare(amount);
    }

    #pragma endregion

    #pragma region ** wflLevel **

    //generate cells for a given level and mark level unlocked if compatible green/red set found
    ACTION unlocklvl(name owner, uint64_t idlevel) {
      require_auth(owner);

      Level::Level level(get_self(), idlevel);
      level.unlockRootLevel(owner);
    }

    //position player to the next level
    //if not yet exists - initialize new locked level in current branch 
    //split pot according to level's branch metadata(`nxtrate`), 
    //make the player a branch winner
    //as new level is locked, winner have 3 tries to unlock it, if no luck - zero-ed in current level
    ACTION nextlvl(name account) {
      require_auth(account);

      Level::PlayerLevel plevel(get_self(), account);
      plevel.nextLevel();
    }

    //reset player's position and retries count in current level's zero cell for a certain payment, 
    //defined by branch rules in its metadata (unjlrate, unjlmin)
    ACTION unjail(name account) {
      require_auth(account);

      Level::PlayerLevel plevel(get_self(), account);
      plevel.unjailPlayer();
    }
        
    #pragma endregion

    #pragma region ** wflPlayer **
        
    //set current root branch for player and position at 1st level
    ACTION switchbrnch(name account, uint64_t idbranch) {
      require_auth(account);

      Player::Player player(get_self(), account);      
      player.switchBranch(idbranch);//position player in root level of the branch
    }
        
    //use try to change position in current level from safe to green. last try will change position automatically
    ACTION tryturn(name account) {
      require_auth(account);

      Player::Player player(get_self(), account);
      player.tryTurn();
    }
    
    //commit position change in current level
    ACTION committurn(name account) {
      require_auth(account);
      
      Player::Player player(get_self(), account);
      player.commitTurn();
    }

    //reset player's GREEN position to SAFE (current level's zero cell) if a player don't want to continue trial of splitting branch or extending it
    ACTION claimgreen(name account) {
      require_auth(account);          

      Player::Player player(get_self(), account);
      player.claimGreen();
    }

    //commit player's position after turn result "red cell" (position player to prev. level's zero)
    ACTION claimred(name account) {
      require_auth(account);

      Player::Player player(get_self(), account);
      player.claimRed();
    }
    
    #pragma endregion
    
    #pragma region ** wflStake **

    ACTION stkaddval(name owner, uint64_t idbranch, asset amount) {
      require_auth(owner);
      
      Branch::Branch branch(get_self(), idbranch);
      branch.addStake(owner, amount);
    }  

    ACTION tipstkhldrs(uint64_t idbranch, asset amount, uint128_t txid) {
      require_auth(get_self());
      
      print("Tipping stakeholders of branch with id <", std::to_string(idbranch), "> with total amount: ", asset{amount}, ".\n");

      Stake::Stake stake(get_self(), 0);
      stake.allocateRevshare(idbranch, amount, txid);
    }

    #pragma endregion

    #pragma region ** wflChannel **
    
    //merge channel balance into channel owner's active balance
    ACTION chnmergebal(name owner) {
      require_auth(owner);
      
      Channel::Channel channel(get_self(), owner);
      channel.mergeBalance();
    }
    
    //revenue share, called as deferred action
    ACTION tipchannel(name channel, asset amount) {
      require_auth(get_self());
      
      print("Tipping channel <", name{channel}, "> with amount: ", asset{amount}, ".\n");

      Channel::Channel chnl(get_self(), channel);
      chnl.addBalance(amount);
    }
    
    #pragma endregion

    #pragma region ** DEBUG **

    //DEBUG: testing cells generation for a given level and meta
    ACTION regencells(uint64_t idlevel) {
      require_auth(get_self());

      Level::Level level(get_self(), idlevel);
      level.regenCells(get_self());
    }

    //DEBUG: testing cell randomizer
    ACTION gencells(uint8_t size) {
      require_auth(get_self());

      Level::Level::debugGenerateCells(get_self(), 1, size);
    }

    //DEBUG: testing level delete
    ACTION teleport(name account, uint64_t idlevel, uint8_t position) {
      require_auth(get_self());

      Player::Player player(get_self(), account);
      player.reposition(idlevel, position);
    }

    //DEBUG actions for branch generation debug 
    ACTION setrootlvl(uint64_t idbranch, uint64_t idrootlvl) {
      require_auth(get_self());

      Branch::Branch branch(get_self(), idbranch);
      branch.setRootLevel(get_self(), idrootlvl);
    }

    //DEBUG: testing level delete
    ACTION rmlevel(uint64_t idlevel) {
      require_auth(get_self());

      Level::Level level(get_self(), idlevel);
      level.rmLevel();
    }

    //DEBUG: testing
    ACTION rmbranch(uint64_t idbranch) {
      require_auth(get_self());

      Branch::Branch branch(get_self(), idbranch);
      branch.rmBranch();
    }

    //DEBUG: testing
    ACTION rmstake(uint64_t idstake) {
      require_auth(get_self());
      
      Stake::Stake stake(get_self(), idstake);
      stake.rmStake();
    }
    
    #pragma endregion
  };
}