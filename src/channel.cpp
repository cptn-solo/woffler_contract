#include <channel.hpp>
#include <player.hpp>
#include <math.h>

namespace Woffler {
  namespace Channel {
    Channel::Channel(name self, name owner) : 
      Entity<channels, DAO, name>(self, owner) {}

    DAO::DAO(channels& _channels, uint64_t _ownerV): 
      Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, _ownerV) {}
    
    DAO::DAO(channels& _channels, channels::const_iterator itr): 
      Accessor<channels, wflchannel, channels::const_iterator, uint64_t>::Accessor(_channels, itr) {}
    
    wflchannel Channel::getChannel() {
      return getEnt<wflchannel>();
    }

    uint8_t Channel::getRate() {
      uint8_t h = getChannel().height;
      if (h < 10) return 0;
      uint8_t retval = floor(log10(h));
      if (retval > Const::maxChannelRate) return Const::maxChannelRate;
      return retval;
    }

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
    
    void Channel::addBalance(asset amount, name payer) {
      update(payer, [&](auto& c) {
        c.balance += amount;     
      });
      print("Channel <", name{_entKey}, "> got ", asset{amount}, ".\n Channel's owner can claim balance ", asset{getChannel().balance}, " using <claimchnl> action. \n");
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
    
    //DEBUG:
    void Channel::rmChannel() {
      remove();
    }
  }
} // namespace Woffler 
