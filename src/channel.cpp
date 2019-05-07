#include <channel.hpp>

namespace Woffler {
    namespace Channel {
        Channel::Channel(name self, name owner) : 
            Entity<channels, DAO, name>(self, owner) {}

        DAO::DAO(channels& _channels, uint64_t _ownerV): 
            Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, _ownerV) {}
        
        void Channel::upsertChannel(name payer) {
            if (isEnt()) {
                update(payer, [&](auto& c) {
                    c.height++;     
                });
            } 
            else {
                create(payer, [&](auto& c) {
                    c.owner = _entKey;
                });
            }
        }

        void Channel::subChannel(name payer) {
            const auto& _channel = getEnt<wflchannel>();
            if (_channel.height > 0) {
                update(payer, [&](auto& c) {
                    if (c.height > 0)
                        c.height--;     
                });
            }
        }
    }
} // namespace Woffler 
