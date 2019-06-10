<h1 class="contract">signup</h1>

### Parameters
Input parameters:

* `account` (player name)
* `referrer` (referrer name)

### Intent
INTENT. The intent of the `{{ signup }}` action is to register account as a game player. Account being registred pays for the RAM for the game registration record. This action can only be called once.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">withdraw</h1>

### Parameters
Input parameters:

* `from` (player name to withdraw from)
* `to` (account name to transfer asset to)
* `amount` (amount to be withdrawn)
* `memo` (memo for the transfer)

### Intent
INTENT. The intent of the `{{ withdraw }}` action is to withdraw player's funds from contract's account.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">forget</h1>

### Parameters
Input parameters:

* `account` (player name)

### Intent
INTENT. The intent of the `{{ forget }}` action is to remove player's account registration record from contract's ledger. This action can only be called once.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">brnchmeta</h1>

### Parameters
Input parameters:

* `owner` (branch presets owner)
* `meta` (branch metadata, or presets)

### Intent
INTENT. The intent of the `{{ brnchmeta }}` action is to create new or update existing set of branch presets. Branch presets is used to create new branches of levels and to configure parameters and gameplay of these levels.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">rmbrmeta</h1>

### Parameters
Input parameters:

* `owner` (branch presets owner)
* `idmeta` (ID of branch presets to be deleted)

### Intent
INTENT. The intent of the `{{ rmbrmeta }}` action is to remove the set of branch presets with specified ID from the contract storage.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">branch</h1>

### Parameters
Input parameters:

* `owner` (player name whos stake will be registred first in the ledger of created branch)
* `idmeta` (ID of branch presets to be used as created branch presets)
* `pot` (amount to be put in the branch stake for the player created a branch)

### Intent
INTENT. The intent of the `{{ branch }}` action is to create new root branch taking an earlier created presets as a template.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">unlocklvl</h1>

### Parameters
Input parameters:

* `owner` (account to unlock the level with - a pretender)
* `idlevel` (ID of the level to be unlocked)

### Intent
INTENT. The intent of the `{{ unlocklvl }}` action is to generate cells for a given level and mark level unlocked if compatible green/red set was found by the pretender. Rules:

* if a level being unlocked is the Root level of Root branch, pretender must own stake in the branch
* if a level being unlocked is Next level or Root level of Split branch, pretender must stand in the previous level, be in NEXT or SPLIT state, and retries count must be > 0. Additional tries are bought by calling `buytries` action.
* upon successful level unlock, NEXT player becomes current branch winner, while SPLIIT player becomes a stakeholder of split branch with stake equal to unlocked level's pot.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">switchbrnch</h1>

### Parameters
Input parameters:

* `account` (player account to be moved to a selected root branch)
* `idbranch` (ID of root branch to move a player to, or zer0 to exit current game)

### Intent
INTENT. The intent of the `{{ switchbrnch }}` action is to select root branch to play on. If succeed, the root level of the selected root branch will become a current level of a player. If zero passed as idbranch, the player returns to initial state without current branch/level selected.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">tryturn</h1>

### Parameters
Input parameters:

* `account` (account of the player going to use his try)

### Intent
INTENT. The intent of the `{{ tryturn }}` action is to use try to change position in current level from safe to green. last try will change position automatically.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">committurn</h1>

### Parameters
Input parameters:

* `account` (account of the player going to commit his current result)

### Intent
INTENT. The intent of the `{{ committurn }}` action is to commit position change in current level.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">claimgreen</h1>

### Parameters
Input parameters:

* `account` (account of the player going to claim his current result)

### Intent
INTENT. The intent of the `{{ claimgreen }}` action is to reset player's GREEN position to SAFE (current level's zero cell) if a player don't want to continue trial of splitting branch or extending it.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">claimred</h1>

### Parameters
Input parameters:

* `account` (account of the player going to claim his current result)

### Intent
INTENT. The intent of the `{{ claimred }}` action is to commit player's position after turn result "red cell" (position player to prev. level's zero cell).

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">stkaddval</h1>

### Parameters
Input parameters:

* `owner` (account of the player going to stake on the branch)
* `idbranch` (ID of the branch to stake on)
* `amount` (amount to be moved from owner's active balance to branch stake)

### Intent
INTENT. The intent of the `{{ stkaddval }}` action is to increase volume of branch starting pot and to add owner's funds to the branch stake:

* cut amount from owner's active balance;
* register amount as owner's stake in specified branch;
* if branch already has a root lvl, add amount to its pot.

Contract account will cut 3% of each amount put on root branch (contract account get 3% in each root branch).

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">claimchnl</h1>

### Parameters
Input parameters:

* `owner` (account of the sales channel's owner)

### Intent
INTENT. The intent of the `{{ claimchnl }}` action is to merge sales channel balance (net income from referrals) into channel owner's active balance.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">nextlvl</h1>

### Parameters
Input parameters:

* `account` (account of the player going to extend current branch with new level)

### Intent
INTENT. The intent of the `{{ nextlvl }}` action is to position player to the next level. If nex level does not yet exists - new level will be initialised in current branch:

* split pot according to level's branch metadata(`nxtrate`);
* make the player a branch winner;
* as new level is locked when created, the winner has 3 tries to unlock it;
* if no free unlock retries left, player can buy another set of retries from his active balance and reset retries count.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">unjail</h1>

### Parameters
Input parameters:

* `account` (account of the player going to make payment for un-jail)

### Intent
INTENT. The intent of the `{{ unjail }}` action is to reset player's position and retries count in current level's zero cell for a certain payment:

* payment rules are defined in the branch metadata of current level (see table [brnchmeta] of the game contract);
* payment is calculated as ([current level's pot]*[unjlrate]/100);
* payment can't be less then ([unjlmin]).

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">revshare</h1>

### Parameters
Input parameters: no input parameters.

### Intent
INTENT. The intent of the `{{ revshare }}` action is to initiate revenue share allocation processing for all unprocessed branchches. One pass of the action will process one hierarchy level of branch trees at a time. Action can be called by any account or script. Contract pays all RAM in course of the action execution (actually, only deferred actions are recreated during this action).

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">tipbranch</h1>

### Parameters
Input parameters:

* `idbranch` (ID of the branch to be processed).

### Intent
INTENT. The intent of the `{{ tipbranch }}` action is to process revenue share on branch, called as deferred action.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">claimbranch</h1>

### Parameters
Input parameters:

* `owner` (account of the branch stakeholder).
* `idbranch` (ID of the branch to claim revenue from).

### Intent
INTENT. The intent of the `{{ claimbranch }}` action is to claim branch stake holder's share of branch revenue.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">takelvl</h1>

### Parameters
Input parameters:

* `account` (account of the player decided take his reward).

### Intent
INTENT. The intent of the `{{ takelvl }}` action is to split level's pot according to level's branch metadata (`tkrate`) and reward player (vesting balance update). Player wait untill the end of `tkintrvl` set with level result upon `takelvl` action is called. Player calls `claimtake` action to move further after `tkintrvl` expires. After `takelvl` player positioned in current level, zero cell, and can repeat his trial of the level.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">claimtake</h1>

### Parameters
Input parameters:

* `account` (account of the player claiming expired TAKE state to be reset).

### Intent
INTENT. The intent of the `{{ claimtake }}` action is to reset player's TAKE position to SAFE (current level's zero cell) after TAKE level result timestamp expired.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">untake</h1>

### Parameters
Input parameters:

* `account` (account of the player cancelling TAKE state and returning vested funds back to level's pot).

### Intent
INTENT. The intent of the `{{ untake }}` action is to return vested balance to level's pot and set player's state back to GREEN.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">splitlvl</h1>

### Parameters
Input parameters:

* `account` (account of the player initiating branch split from current level. State of the player must be GREEN).

### Intent
INTENT. The intent of the `{{ splitlvl }}` action is to:

* make subbranch with locked root level
* split level's pot according to level's branch metadata (`spltrate`, `stakemin`)
* as new level is locked, splitter have 3 tries to unlock it using `unlocklvl` action and become split branch stakeholder. If no luck - additional tries can be bought by calling `buytries` action.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">buytries</h1>

### Parameters
Input parameters:

* `account` (account of the player to be charged for retries count reset. State of the player must be NEXT or SPLIT, retries count must be 0).

### Intent
INTENT. The intent of the `{{ buytries }}` action to reset retries count while trying to unlock new or split branch:

* if no free unlock retries left, player can buy another set of retries from his active balance and reset retries count
* price is calculated according to level's branch metadata (`triesrate`, `triesmin`)

### Term
TERM. This Contract expires at the conclusion of code execution.