<h1 class="contract">signup</h1>

### Parameters
Input parameters:

* `account` (player name)
* `referrer` (referrer name)

### Intent
INTENT. The intent of the `{{ signup }}` action is to register account as a game player. Account being registred pays for the RAM for the game registration record. This action can only be called once.

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

<h1 class="contract">switchbrnch</h1>

### Parameters
Input parameters:

* `account` (player account to be moved to a selected root branch)
* `idbranch` (ID of root branch to move a player to)

### Intent
INTENT. The intent of the `{{ switchbrnch }}` action is to select root branch to play on. If succeed, the root level of the selected root branch will become a current level of a player.

### Term
TERM. This Contract expires at the conclusion of code execution.