#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>

namespace Woffler {
    namespace Player {
        Player::Player(name self, name player): _players(_self, _self.value) {
            this->_self = self;
            this->_player = player;
        }

        void Player::createPlayer() {

        }
    }
    
} // namespace Woffler 
