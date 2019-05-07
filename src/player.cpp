#include <player.hpp>

namespace Woffler {
    namespace Player {
        Player::Player(name self, name account) : 
            Entity<players, DAO, name>(self, account) {}
        
        DAO::DAO(players& _players, uint64_t _playerV): 
            Accessor<players, wflplayer, players::const_iterator, uint64_t>::Accessor(_players, _playerV) {}

        name Player::getChannel() {
            return getEnt<wflplayer>().channel;
        }   

        void Player::createPlayer(name payer, name referrer) {
            check(
                _entKey == _self || referrer != _entKey, //only contract account can be register by his own
                "One can not be a sales channel for himself"
            );

            //channel's account must exist at the moment of player signup unless channel isn't the contract itself
            if (referrer != _entKey) 
                checkReferrer(referrer);

            //account can't be registred twice
            checkNoPlayer();

            create(payer, [&](auto& p) {
                p.account = _entKey;
                p.channel = referrer;
            });
            auto _p = getEnt<wflplayer>();
        }

        void Player::addBalance(asset amount, name payer) {    
            checkPlayer();
            update(payer, [&](auto& p) {
                p.activebalance += amount;     
            });      
        }

        void Player::subBalance(asset amount, name payer) {
            checkBalanceCovers(amount);
            update(payer, [&](auto& p) {
                p.activebalance -= amount;
            });
        }

        void Player::rmAccount() {
            checkBalanceZero();
            checkNotReferrer();          
            remove();
        }

        void Player::switchRootLevel(uint64_t idlvl) {
            //position player in root level of the branch
            update(_entKey, [&](auto& p) {
                p.idlvl = idlvl;
                p.triesleft = Const::retriesCount;     
                p.levelresult = Const::playerstate::SAFE;
                p.tryposition = 0;
                p.currentposition = 0;
            });
        }

        void Player::useTry() {
            auto p = getEnt<wflplayer>();
            useTry(p.tryposition);  
        }

        void Player::useTry(uint8_t position) {
            update(_entKey, [&](auto& p) {
                p.tryposition = position;
                p.triesleft -= 1;
            });
        }

        void Player::commitTurn(Const::playerstate result) {
            auto player = getEnt<wflplayer>();
            update(_entKey, [&](auto& p) {
                p.currentposition = player.tryposition;
                p.levelresult = result;
                p.resulttimestamp = Utils::now();
                p.triesleft = Const::retriesCount;
            });
        }

        void Player::resetPositionAtLevel(uint64_t idlvl) {
            update(_entKey, [&](auto& p) {
                p.idlvl = idlvl;
                p.tryposition = 0;
                p.currentposition = 0;
                p.levelresult = Const::playerstate::SAFE;
                p.resulttimestamp = 0;
                p.triesleft = Const::retriesCount;
            });
        }

        bool Player::isPlayer() {
            return isEnt();
        }

        void Player::checkReferrer(name referrer) {
            check(
                isEnt(referrer),
                string("Account ") + referrer.to_string() + string(" is not registred in game conract.")
            );
        }

        void Player::checkNotReferrer() {
            auto player = getEnt<wflplayer>();
            auto idxchannel = getIndex<"bychannel"_n>();
            auto itrchannel = idxchannel.find(_entKey.value);
            check(
                itrchannel == idxchannel.end(),
                string("Can't remove account ") + _entKey.to_string() + string(" as it is registred as a referrer for other accounts.")
            );
            return ;
        }

        void Player::checkPlayer() {
            check(
                isEnt(),
                string("Account ") + _entKey.to_string() + string(" is not registred in game conract. Please signup or send some funds to ") + _self.to_string() + string(" first.")
            ); 
        }

        void Player::checkNoPlayer() {      
            check(
                !isEnt(), 
                string("Account ") + _entKey.to_string() + string(" is already registred.")
            );
        }

        void Player::checkActivePlayer() {
            auto p = getEnt<wflplayer>();
            check(
                p.idlvl != 0,
                "First select branch to play on with action switchbrnch."
            );
        }

        void Player::checkState(Const::playerstate state) {
            auto p = getEnt<wflplayer>();
            checkActivePlayer();    
            check(
                p.levelresult == state,
                string("Player current level resutl must be '") + std::to_string(state) + string("'.")
            );
        }

        void Player::checkBalanceCovers(asset amount) {
            auto p = getEnt<wflplayer>();
            check(
                p.activebalance >= amount, 
                string("Not enough active balance in your account. Current active balance: ") + p.activebalance.to_string().c_str() 
            );    
        }

        void Player::checkBalanceZero() {
            auto p = getEnt<wflplayer>();
            check(//warning! works only for records, emplaced in contract's host scope
                p.activebalance == asset{0, Const::acceptedSymbol},
                string("Please withdraw funds first. Current active balance: ") + p.activebalance.to_string().c_str()
            );
        }

        void Player::checkSwitchBranchAllowed() {
            auto p = getEnt<wflplayer>();
            check(
                p.levelresult == Const::playerstate::INIT ||
                p.levelresult == Const::playerstate::SAFE,
                "Player can switch branch only from safe locations."
            );
        }

        void Player::checkLevelUnlockTrialAllowed(uint64_t idlvl) {
            auto p = getEnt<wflplayer>();
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
    }
} // namespace Woffler 
