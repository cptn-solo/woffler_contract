#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>
#include <channel.hpp>

namespace Woffler {
    namespace Player {
        Player::Player(name self, name account) : _players(self, self.value) {
            this->_self = self;
            this->_player = account;
            
            DAO d(_players, _player);
            this->_dao = &d;
        }        
        
        Player::~Player() {
            delete this->_dao;
            this->_dao = NULL;
        }

        void Player::createPlayer(name payer, name referrer) {
            auto _referrer = (referrer ? referrer : _self);
            check(
                _player == _self || _referrer != _player, //only contract account can be register by his own
                "One can not be a sales channel for himself"
            );

            //channel's account must exist at the moment of player signup unless channel isn't the contract itself
            if (_referrer != _player) 
                checkReferrer(_referrer);

            //account can't be registred twice
            checkNoPlayer();

            _dao->create(payer, [&](auto& p) {
                p.account = _player;
                p.channel = _referrer;
            });

            Channel::Channel channel = Channel::Channel(_self, _referrer);
            //contract pays for the sales channels' records RAM:
            channel.upsertChannel(_self);
        }

        void Player::checkReferrer(name referrer) {
            check(
                _dao->isAccountRegistred(referrer),
                string("Account ") + referrer.to_string() + string(" is not registred in game conract.")
            );
        }

        void Player::checkPlayer() {
            check(
                _dao->isAccountRegistred(),
                string("Account ") + _player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
            ); 
        }

        void Player::checkNoPlayer() {      
            check(
                !_dao->isAccountRegistred(), 
                string("Account ") + _player.to_string() + string(" is already registred.")
            );
        }

        void Player::checkActivePlayer() {
            auto p = _dao->getPlayer();
            check(
                p.idlvl != 0,
                "First select branch to play on with action switchbrnch."
            );
        }

        void Player::checkState(Const::playerstate state) {
            auto p = _dao->getPlayer();
            checkActivePlayer();    
            check(
                p.levelresult == state,
                string("Player current level resutl must be '") + std::to_string(state) + string("'.")
            );
        }

        void Player::checkBalanceCovers(asset amount) {
            auto p = _dao->getPlayer();
            check(
                p.activebalance >= amount, 
                string("Not enough active balance in your account. Current active balance: ") + p.activebalance.to_string().c_str() 
            );    
        }

        void Player::checkBalanceZero() {
            auto p = _dao->getPlayer();
            check(//warning! works only for records, emplaced in contract's host scope
                p.activebalance == asset{0, Const::acceptedSymbol},
                string("Please withdraw funds first. Current active balance: ") + p.activebalance.to_string().c_str()
            );
        }

        void Player::checkSwitchBranchAllowed() {
            auto p = _dao->getPlayer();
            check(
                p.levelresult == Const::playerstate::INIT ||
                p.levelresult == Const::playerstate::SAFE,
                "Player can switch branch only from safe locations."
            );
        }

        void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
            auto p = _dao->getPlayer();
            check(
                p.idlvl == idlvl,
                "Player must be at previous level to unlock next one."
            );
            check(
                (
                p.levelresult == Const::playerstate::GREEN || 
                p.levelresult == Const::playerstate::TAKE
                ),
                "Player can unlock level only from GREEN position"
            );
            check(
                p.triesleft >= 1,
                "No retries left"
            );
        }        
        
        #pragma region ** DAO implementation **

        DAO::DAO(players& players, name player): _pitr(players.find(player.value)), _players{players} {}
        
        template<typename Lambda>
        void DAO::create(name payer, Lambda&& creator) {
            _pitr = _players.emplace(payer, std::forward<Lambda&&>(creator)); 
        }

        template<typename Lambda>
        void DAO::update(name payer, Lambda&& updater) {
            _players.modify(_pitr, payer, std::forward<Lambda&&>(updater)); 
        }

        /*** 
          usage (check for existance before use!!!):
          auto p = getPlayer();
          print("player: ", name{p.account});
        */
        const wflplayer& DAO::getPlayer() {
            check(isAccountRegistred(), "Account not registred.");
            return *_pitr;
        }
            
        bool DAO::isAccountRegistred() {      
            return _pitr != _players.end();
        }

        bool DAO::isAccountRegistred(name account) {      
            return _players.find(account.value) != _players.end();
        }

        #pragma endregion
    }
    
} // namespace Woffler 
