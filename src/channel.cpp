#include <channel.hpp>
#include <player.hpp>
#include <math.h>

namespace Woffler {
  namespace Channel {
    uint8_t Channel::getRate() {
      uint8_t h = getChannel().height;
      if (h < 10) return 0;
      uint8_t retval = floor(log10(h));
      if (retval > Const::maxChannelRate) return Const::maxChannelRate;
      return retval;
    }

    void Channel::upsertChannel(const name& payer) {
      if (isEnt()) {
        _channel = update(payer, [&](auto& c) {
          c.height++;     
        });
      } 
      else {
        _channel = create(payer, [&](auto& c) {
          c.owner = _entKey;
        });
      }
    }

    void Channel::subChannel(const name& payer) {
      if (_channel.height > 0) {
        _channel = update(payer, [&](auto& c) {
          if (c.height > 0)
            c.height--;     
        });
      }
    }
    
    void Channel::addBalance(const asset& amount, const name& payer) {
      _channel = update(payer, [&](auto& c) {
        c.balance += amount;     
      });
      print("Channel <", name{_entKey}, "> got ", asset{amount}, ".\n Channel's owner can claim balance ", asset{getChannel().balance}, " using <claimchnl> action. \n");
    }

    void Channel::mergeBalance() {
      auto amount = _channel.balance;
      _channel = update(_entKey, [&](auto& c) {
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
