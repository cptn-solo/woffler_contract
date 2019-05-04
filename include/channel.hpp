#include <utils.hpp>
#include <constants.hpp>
#include <accessor.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;
    
    namespace Channel {
        //sales channels with user counter and current revenue balance available to merge into channel owner's balance
        typedef struct
        [[eosio::table("channels"), eosio::contract("woffler")]]
        wflchannel {
            name owner;
            uint64_t height = 1;
            asset balance = asset{0, Const::acceptedSymbol};
            
            uint64_t primary_key() const { return owner.value; }
        } wflchannel;
        typedef multi_index<"channels"_n, wflchannel> channels; 
        
        struct DAO: Accessor<channels, wflchannel, channels::const_iterator, uint64_t>  {
            DAO(channels& channels, uint64_t ownerV);
        };

        class Channel {
            public:

                Channel(name self, name owner);
                ~Channel();
                
                void upsertChannel(name payer);

            private:

                name _self;
                name _owner;            
                channels _channels;     
                DAO* _dao = NULL;
        };
    }
}
