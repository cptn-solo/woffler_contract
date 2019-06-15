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
      //TODO: add withdraw if balance >0
      //TODO: add rmStake (+recalc branch total stake)
      //TODO: add rm channel (+move referrals to contract, now just forbidden if account being forgotten has referrals)

      Player::Player player(get_self(), account);
      player.rmAccount();      
    }

    //can be called from other actions as "additional" task to minimize manual `revshare` calls
    void processPendingRevshare() {
      action(
        permission_level{get_self(),"active"_n},
        get_self(),
        "revshare"_n,
        std::make_tuple()
      ).send();
    }

    //initiate processing of unprocessed branchches for 1 hierarchy level at a time. can be called by any account or script
    ACTION revshare() {
      auto _self = get_self();

      Branch::branches _branches(_self, _self.value);
      auto idx = _branches.get_index<"byprocessed"_n>();
      auto itr = idx.find(0);//only unprocessed
      uint8_t treshold = 0; //limit processing by 10 items per run
      transaction out{};
      while(itr != idx.end() && treshold < 10) {
        treshold++;
        out.actions.emplace_back(permission_level{_self, "active"_n}, _self, "tipbranch"_n, std::make_tuple(itr->id));
        print("revshare branch: <", std::to_string(itr->id), "> \n");
        out.delay_sec = treshold;
        out.send(Utils::deferredTXId(itr->id), _self);
        itr++;
      }
    }
    
    #pragma endregion
    
    #pragma region ** wflBranchMeta **

    //create meta for root branch(es) - active balance must cover at least.
    //owner pays for ram to avoid spamming via branch meta creation.
    //only owner can modify branch metadata.
    ACTION brnchmeta(name owner, uint64_t idmeta, BranchMeta::wflbrnchmeta meta) {
      require_auth(owner);

      BranchMeta::BranchMeta branchmeta(get_self(), idmeta);
      branchmeta.upsertBranchMeta(owner, meta);
    }

    //remove branch meta owned
    ACTION rmbrmeta(name owner, uint64_t idmeta) {
      require_auth(owner);

      BranchMeta::BranchMeta branchmeta(get_self(), idmeta);
      if (owner != get_self())
        branchmeta.removeBranchMeta(owner);
      else branchmeta.removeBranchMeta();
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
    
    //revenue share, called as deferred action
    ACTION tipbranch(uint64_t idbranch) {
      auto self = get_self();
      require_auth(self);
      
      print("Tipping branch: <", std::to_string(idbranch), "> \n");

      Branch::Branch branch(self, idbranch);
      branch.allocateRevshare();
    }

    #pragma endregion

    #pragma region ** wflLevel **

    //generate cells for a given level and mark level unlocked if compatible green/red set found
    ACTION unlocklvl(name account, uint64_t idlevel) {
      require_auth(account);

      Level::Level level(get_self(), idlevel);
      level.unlockLevel(account);
    }
    
    //use try to change position in current level from safe to green. last try will change position automatically
    ACTION tryturn(name account) {
      require_auth(account);

      Level::PlayerLevel plevel(get_self(), account);
      plevel.tryTurn();
    }
    
    //commit position change in current level
    ACTION committurn(name account) {
      require_auth(account);
      
      Level::PlayerLevel plevel(get_self(), account);
      plevel.commitTurn();
    }

    //reset player's GREEN/NEXT/SPLIT state to SAFE (current level's zero cell) if a player don't want to continue trial of splitting branch or extending it
    ACTION claimsafe(name account) {
      require_auth(account);          

      Level::PlayerLevel plevel(get_self(), account);
      plevel.claimSafe();
    }

    //commit player's position after turn result "red cell" (position player to prev. level's zero)
    ACTION claimred(name account) {
      require_auth(account);

      Level::PlayerLevel plevel(get_self(), account);
      plevel.claimRed();
    }

    //reset player's TAKE position to SAFE (current level's zero cell) after TAKE level result timestamp expired
    ACTION claimtake(name account) {
      require_auth(account);

      Level::PlayerLevel plevel(get_self(), account);
      plevel.claimTake();
    }
    
    //return vested balance to level's pot and set state back to GREEN
    ACTION untake(name account) {
      require_auth(account);
      
      Level::PlayerLevel plevel(get_self(), account);
      plevel.cancelTake();
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

    //split level's pot according to level's branch metadata (`tkrate`) and reward player (vesting balance update)
    //player wait untill the end of `tkintrvl` set with level result upon `takelvl`
    //player calls `claimtake` to move further after `tkintrvl` expires - then zero-ed in current level
    ACTION takelvl(name account) {
      require_auth(account);

      Level::PlayerLevel plevel(get_self(), account);
      plevel.takeReward();
    }

    //make subbranch with locked root level
    //split level's pot according to level's branch metadata (`spltrate`, `potmin`)
    //make the player a stakeholder of new subbranch, share is defined by level's branch metadata (`stkrate`, `stkmin`)
    //as new level is locked, splitter have 3 tries to unlock it, if no luck - zero-ed in current level
    ACTION splitlvl(name account) {
      require_auth(account);
      
      Level::PlayerLevel plevel(get_self(), account);
      plevel.splitLevel();
    }

    //if no free unlock retries left, player can buy retries from his active balance to reset retries count  
    //price is calculated according to level's branch metadata `unjail` price
    ACTION buytries(name account) {
      require_auth(account);
      
      Level::PlayerLevel plevel(get_self(), account);
      plevel.buyRetries();
    }

    #pragma endregion

    #pragma region ** wflPlayer **
        
    //set current root branch for player and position at 1st level
    ACTION switchbrnch(name account, uint64_t idbranch) {
      require_auth(account);

      Player::Player player(get_self(), account);      
      player.switchBranch(idbranch);//position player in root level of the branch
    }
        
    
    #pragma endregion
    
    #pragma region ** wflStake **

    ACTION stkaddval(name owner, uint64_t idbranch, asset amount) {
      require_auth(owner);
      
      Branch::Branch branch(get_self(), idbranch);
      branch.addStake(owner, amount);
    }  

    //claim branch stake holder's share of branch revenue
    ACTION claimbranch(name owner, uint64_t idbranch) {
      require_auth(owner);      

      Stake::Stake stake(get_self(), 0);
      stake.claimRevenue(owner, idbranch);
    }

    #pragma endregion

    #pragma region ** wflChannel **
    
    //merge channel balance into channel owner's active balance
    ACTION claimchnl(name owner) {
      require_auth(owner);
      
      Channel::Channel channel(get_self(), owner);
      channel.mergeBalance();
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

    //DEBUG: testing
    ACTION rmplayer(name account) {
      require_auth(get_self());
      
      Player::Player player(get_self(), account);
      player.rmPlayer();
    }

    //DEBUG: testing
    ACTION rmpchannel(name account) {
      require_auth(get_self());
      
      Channel::Channel channel(get_self(), account);
      channel.rmChannel();
    }
    
    #pragma endregion
  };
}