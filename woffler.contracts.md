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

<h1 class="contract">rootlvl</h1>

### Parameters
Input parameters:

* `owner` (account creating a root level. Must have stake in the root branch of the created level)
* `idbranch` (ID of branch where the root level is created)

### Intent
INTENT. The intent of the `{{ rootlvl }}` action is to create new root level for a given root branch. All stake of the branch is put into the pot of the created level.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">unlocklvl</h1>

### Parameters
Input parameters:

* `owner` (account to unlock the level with - a pretender)
* `idlevel` (ID of the level to be unlocked)

### Intent
INTENT. The intent of the `{{ unlocklvl }}` action is to generate cells for a given level and mark level unlocked if compatible green/red set was found by the pretender. If a level being unlocked is the Root level of the Root branch, pretender must own stake in the branch. If a level is general one, pretender must stand in the previous level and be Green.

### Term
TERM. This Contract expires at the conclusion of code execution.

<h1 class="contract">switchbrnch</h1>

### Parameters
Input parameters:

* `account` (player account to be moved to a selected root branch)
* `idbranch` (ID of root branch to move a player to)

### Intent
INTENT. The intent of the `{{ switchbrnch }}` action is to select root branch to play on. If succeed, the root level of the selected root branch will become a current level of a player.

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
INTENT. The intent of the `{{ stkaddval }}` action is to ncrease volume of root branch starting pot and to add owner's funds to the branch stake:

* cut amount from owner's active balance;
* register amount as owner's stake in specified branch;
* if branch already has a root lvl, add amount to its pot.

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
* as new level is locked when created, the winner has 3 tries to unlock it, if no luck - positioned to current level's zero cell.

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