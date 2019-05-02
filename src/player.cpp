#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>

namespace Woffler {
    namespace Player {
        Player::Player(name self, name account): 
            _players(_self, _self.value), 
            _pitr(_players.find(_player.value)) 
        {
            this->_self = self;
            this->_player = account;
        }
        
        const Player::wflplayer& Player::getPlayer() {
            return *_pitr;
        }
        
        void Player::checkNoPlayer() {      
            check(
                _pitr == _players.end(), 
                "Account already exists"
            );
        }

        void Player::createPlayer(name payer, name channel) {
            auto _channel = (channel ? channel : _self);
            check(
                _player == _self || channel != _player, //only contract account can be register by his own
                "One can not be a sales channel for himself"
            );
            
            checkNoPlayer();

            _players.emplace(payer, [&](auto& p) {
                p.account = _player;
                p.channel = _channel;
            });
        }
    }
    
} // namespace Woffler 
