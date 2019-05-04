#include <utils.hpp>
#include <constants.hpp>
#include <player.hpp>

namespace Woffler {
    namespace Player {
        Player::Player(name self, name account) : _players(self, self.value) {
            this->_self = self;
            this->_player = account;
            
            DAO d(_players, _player.value);
            this->_dao = &d;
        }        
        
        Player::~Player() {
            delete this->_dao;
            this->_dao = NULL;
        }

        void Player::createPlayer(name payer, name referrer) {
            check(
                _player == _self || referrer != _player, //only contract account can be register by his own
                "One can not be a sales channel for himself"
            );

            //channel's account must exist at the moment of player signup unless channel isn't the contract itself
            if (referrer != _player) 
                checkReferrer(referrer);

            //account can't be registred twice
            checkNoPlayer();

            _dao->create(payer, [&](auto& p) {
                p.account = _player;
                p.channel = referrer;
            });
        }

        void Player::addBalance(asset amount, name payer) {    
            checkPlayer();
            _dao->update(payer, [&](auto& p) {
                p.activebalance += amount;     
            });      
        }

        void Player::subBalance(asset amount, name payer) {
            checkBalanceCovers(amount);
            _dao->update(payer, [&](auto& p) {
                p.activebalance -= amount;     
            });     
        }

        void Player::rmAccount() {
            checkBalanceZero();
            _dao->remove();
        }

        void Player::switchRootLevel(uint64_t idlvl) {
            //position player in root level of the branch
            _dao->update(_player, [&](auto& p) {
                p.idlvl = idlvl;
                p.triesleft = Const::retriesCount;     
                p.levelresult = Const::playerstate::SAFE;
                p.tryposition = 0;
                p.currentposition = 0;
            });
        }

        void Player::useTry() {
            auto p = _dao->getEnt();
            useTry(p.tryposition);  
        }

        void Player::useTry(uint8_t position) {
            _dao->update(_player, [&](auto& p) {
                p.tryposition = position;
                p.triesleft -= 1;
            });
        }

        void Player::commitTurn(Const::playerstate result) {
            auto player = _dao->getEnt();
            _dao->update(_player, [&](auto& p) {
                p.currentposition = player.tryposition;
                p.levelresult = result;
                p.resulttimestamp = Utils::now();
                p.triesleft = Const::retriesCount;
            });
        }

        void Player::resetPositionAtLevel(uint64_t idlvl) {
            _dao->update(_player, [&](auto& p) {
                p.idlvl = idlvl;
                p.tryposition = 0;
                p.currentposition = 0;
                p.levelresult = Const::playerstate::SAFE;
                p.resulttimestamp = 0;
                p.triesleft = Const::retriesCount;
            });
        }

        void Player::checkReferrer(name referrer) {
            check(
                _dao->isAccountRegistred(referrer),
                string("Account ") + referrer.to_string() + string(" is not registred in game conract.")
            );
        }

        void Player::checkPlayer() {
            check(
                _dao->isEnt(),
                string("Account ") + _player.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
            ); 
        }

        void Player::checkNoPlayer() {      
            check(
                !_dao->isEnt(), 
                string("Account ") + _player.to_string() + string(" is already registred.")
            );
        }

        void Player::checkActivePlayer() {
            auto p = _dao->getEnt();
            check(
                p.idlvl != 0,
                "First select branch to play on with action switchbrnch."
            );
        }

        void Player::checkState(Const::playerstate state) {
            auto p = _dao->getEnt();
            checkActivePlayer();    
            check(
                p.levelresult == state,
                string("Player current level resutl must be '") + std::to_string(state) + string("'.")
            );
        }

        void Player::checkBalanceCovers(asset amount) {
            auto p = _dao->getEnt();
            check(
                p.activebalance >= amount, 
                string("Not enough active balance in your account. Current active balance: ") + p.activebalance.to_string().c_str() 
            );    
        }

        void Player::checkBalanceZero() {
            auto p = _dao->getEnt();
            check(//warning! works only for records, emplaced in contract's host scope
                p.activebalance == asset{0, Const::acceptedSymbol},
                string("Please withdraw funds first. Current active balance: ") + p.activebalance.to_string().c_str()
            );
        }

        void Player::checkSwitchBranchAllowed() {
            auto p = _dao->getEnt();
            check(
                p.levelresult == Const::playerstate::INIT ||
                p.levelresult == Const::playerstate::SAFE,
                "Player can switch branch only from safe locations."
            );
        }

        void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
            auto p = _dao->getEnt();
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

        DAO::DAO(players& _players, uint64_t _playerV): 
            Accessor<players, wflplayer, players::const_iterator, uint64_t>::Accessor(_players, _playerV) {
        }
        
        void DAO::remove() {
            _idx.erase(_itr);      
        }

        bool DAO::isAccountRegistred(name account) {      
            return _idx.find(account.value) != _idx.end();
        }

        #pragma endregion
    }
    
} // namespace Woffler 
