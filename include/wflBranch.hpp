#include <entity.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;

    namespace Branch {
        //branches for levels
        typedef struct
        [[eosio::table("branches"), eosio::contract("woffler")]]
        wflbranch {
            uint64_t id;
            uint64_t idrootlvl = 0;
            uint64_t idparent = 0;
            uint64_t idmeta;
            name winner;
            uint64_t generation = 1;

            uint64_t primary_key() const { return id; }
            uint64_t get_idmeta() const { return idmeta; }
        } wflbranch;

        typedef multi_index<"branches"_n, wflbranch,
            indexed_by<"bymeta"_n, const_mem_fun<wflbranch, uint64_t, &wflbranch::get_idmeta>>
        > branches;

        class DAO: Accessor<branches, wflbranch, branches::const_iterator, uint64_t>  {
            DAO(branches& _branches, uint64_t idbranch);
            static uint64_t keyValue(uint64_t idbranch) {
                return idbranch;
            }

            bool isIndexedByMeta(uint64_t idmeta) {
                auto idxbymeta = _branches.get_index<name("bymeta")>();
                auto itrbymeta = idxbymeta.find(idmeta);  
                return itrbymeta != idxbymeta.end();
            }
        };

        class Branch: Entity<branches, DAO, uint64_t> {
            public:
            Branch(name self, uint64_t idbranch);                
            void checkBranch();
            void checkStartBranch();
            void checkEmptyBranch();
            void checkBranchMetaUsage(uint64_t idmeta);
            
            void createBranch(name payer, uint64_t metaid);
            void setRootLevel(name payer, uint64_t rootlvlid);

            bool isIndexedByMeta(uint64_t idmeta);
        };
    }