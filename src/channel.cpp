#include <channel.hpp>

namespace Woffler {
    namespace Channel {
        Channel::Channel(name self, name owner) : Entity<channels, DAO, name>(self, owner) {
        }
        
        void Channel::upsertChannel(name payer) {
            if (_dao->isEnt()) {
                _dao->update(payer, [&](auto& c) {
                    c.height++;     
                });
            } 
            else {
                _dao->create(payer, [&](auto& c) {
                    c.owner = _entKey;
                });
            }
        }

        void Channel::subChannel(name payer) {
            const auto& _channel = _dao->getEnt();
            if (_channel.height > 0) {
                _dao->update(payer, [&](auto& c) {
                    if (c.height > 0)
                        c.height--;     
                });
            }
        }

        DAO::DAO(channels& _channels, uint64_t _ownerV): 
            Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, _ownerV) {
        }
    }
} // namespace Woffler 
