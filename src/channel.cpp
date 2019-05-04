#include <utils.hpp>
#include <constants.hpp>
#include <channel.hpp>

namespace Woffler {
    namespace Channel {
        Channel::Channel(name self, name owner) : _channels(self, self.value) {
            this->_self = self;
            this->_owner = owner;
            
            DAO d(_channels, _owner.value);
            this->_dao = &d;
        }
        
        Channel::~Channel() {
            delete this->_dao;
            this->_dao = NULL;
        }
        
        void Channel::upsertChannel(name payer) {
            
            if (_dao->isEnt()) {
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

        DAO::DAO(channels& _channels, uint64_t _ownerV): 
            Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, _ownerV) {
        }
    }
} // namespace Woffler 
