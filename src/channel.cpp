#include <utils.hpp>
#include <constants.hpp>
#include <channel.hpp>

namespace Woffler {
    namespace Channel {
        Channel::Channel(name self, name owner) : _channels(self, self.value) {
            this->_self = self;
            this->_owner = owner;
            
            DAO d(_channels, _owner);
            this->_dao = &d;
        }
        
        Channel::~Channel() {
            delete this->_dao;
            this->_dao = NULL;
        }
        
        void Channel::upsertChannel(name payer) {
            
            if (_dao->isRegistred()) {
                _dao->update(payer, [&](auto& c) {
                    c.height++;     
                });
            } 
            else {
                _dao->create(payer, [&](auto& c) {
                    c.owner = _owner;
                });
            }
        }

        DAO::DAO(channels& channels, name owner): _citr(channels.find(owner.value)), _channels{channels} {}
        
        template<typename Lambda>
        void DAO::create(name payer, Lambda&& creator) {
            _citr = _channels.emplace(payer, std::forward<Lambda&&>(creator)); 
        }

        template<typename Lambda>
        void DAO::update(name payer, Lambda&& updater) {
            _channels.modify(_citr, payer, std::forward<Lambda&&>(updater)); 
        }

        bool DAO::isRegistred() {      
            return _citr != _channels.end();
        }

        const wflchannel& DAO::getChannel() {
            check(isRegistred(), "Channel not registred.");
            return *_citr;
        }            
    }
    
} // namespace Woffler 
