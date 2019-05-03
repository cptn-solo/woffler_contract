#include <utils.hpp>
#include <constants.hpp>

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
        
        class Channel {
            public:

                Channel(name self, name owner);
                void upsertChannel(name payer);

            private:

                name _self;
                name _owner;            
                channels _channels;     
        };

        struct DAO {
            public:

                DAO(channels& channels, name owner);      

                template<typename Lambda>
                void create(name payer, Lambda&& creator);

                template<typename Lambda>
                void update(name payer, Lambda&& updater);
                
                bool isRegistred();
                const wflchannel& getChannel();
            
            private:

                channels& _channels;     
                channels::const_iterator _citr;
        };

    }
}
