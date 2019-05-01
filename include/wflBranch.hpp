#pragma once
#include <utils.hpp>
using namespace eosio;
using std::string;

class Branch {
    public:
    Branch(name self, uint64_t idbranch);

    uint64_t idbranch = 0;
    uint64_t idrootlvl = 0;
    uint64_t idparent = 0;
    uint64_t idmeta = 0;
    name winner = name();
    uint64_t generation = 1;
            
    void checkBranch();
    void checkStartBranch();
    void checkEmptyBranch();
    void checkBranchMetaUsage(uint64_t idmeta);
    
    void createBranch(name payer, uint64_t metaid);
    void setRootLevel(name payer, uint64_t rootlvlid);
    
    private:
    //branches for levels
    TABLE wflbranch {
        uint64_t id;
        uint64_t idrootlvl = 0;
        uint64_t idparent = 0;
        uint64_t idmeta;
        name winner;
        uint64_t generation = 1;

        uint64_t primary_key() const { return id; }
        uint64_t get_idmeta() const { return idmeta; }
    };
    typedef multi_index<"branches"_n, wflbranch,
        indexed_by<"bymeta"_n, const_mem_fun<wflbranch, uint64_t, &wflbranch::get_idmeta>>
    > branches;

    template<typename Lambda>
    void updateState(name payer, Lambda&& updater);

    name _self;
    uint64_t _idbranch;
    branches _branches;
};
