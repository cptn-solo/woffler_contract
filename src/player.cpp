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
            if (_referrer != _player) {
                check(
                    _dao->isAccountRegistred(_referrer),
                    string("Account ") + _referrer.to_string() + string(" is not registred in game conract.")
                );
            } 

            check(
                !_dao->isAccountRegistred(),
                string("Account ") + _player.to_string() + string(" is already registred.")
            );

            _dao->create(payer, [&](auto& p) {
                p.account = _player;
                p.channel = _referrer;
            });

            Channel::Channel channel = Channel::Channel(_self, _referrer);
            //contract pays for the sales channels' records RAM:
            channel.upsertChannel(_self);
        }
                
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
    }
    
} // namespace Woffler 
