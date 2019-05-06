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