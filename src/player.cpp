#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>

namespace Woffler {
    namespace Player {
        Player::Player(name self, name account) : _players(self, self.value) {
            this->_self = self;
            this->_player = account;
            //this->_pitr = _players.find(account.value);
        }        
        
        void Player::createPlayer(name payer, name channel) {
            check(
                _player == _self || channel != _player, //only contract account can be register by his own
                "One can not be a sales channel for himself"
            );

            auto _channel = (channel ? channel : _self);

            DAO _dao(_players, _player);
            _dao.checkNoPlayer();
            _dao.create(payer, [&](auto& p) {
                p.account = _player;
                p.channel = _channel;
            });
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
            return *_pitr;
        }
            
        void DAO::checkNoPlayer() {      
            check(
                _pitr == _players.end(), 
                "Account already exists"
            );
        }
    }
    
} // namespace Woffler 
