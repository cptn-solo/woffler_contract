#include <channel.hpp>
#include <player.hpp>

namespace Woffler {
  namespace Channel {
    Channel::Channel(name self, name owner) : 
      Entity<channels, DAO, name>(self, owner) {}

    DAO::DAO(channels& _channels, uint64_t _ownerV): 
      Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, _ownerV) {}
    
    DAO::DAO(channels& _channels, channels::const_iterator itr): 
      Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, itr) {}
    
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

    void Channel::mergeBalance() {
      auto achannel = getEnt<wflchannel>();      
      auto amount = achannel.balance;

      update(_entKey, [&](auto& c) {
        c.balance = asset{0, Const::acceptedSymbol};     
      });

      Player::Player player(_self, _entKey);
      player.addBalance(amount, _entKey);      
    }
  }
} // namespace Woffler 
