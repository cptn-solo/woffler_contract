#pragma once
#include <utils.hpp>
#include <constants.hpp>
#include <accessor.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;

    //types of multi-index, accessor and an entity being indexed and accessed
    template<typename Idx, typename A, typename PK>
    class Entity {
        public:

            Entity(name self, PK entKey);
            ~Entity();
            
        protected:

            name _self;
            PK _entKey;            
            Idx _idx;     
            A* _dao = NULL;
    };

    template<typename Idx, typename A, typename PK>
    Entity<Idx, A, PK>::Entity(name self, PK entKey): _idx(self, self.value) {
        this->_self = self;
        this->_entKey = entKey;
        
//        A d;
        this->_dao = new A(_idx, A::keyValue(entKey));
    }

    template<typename Idx, typename A, typename PK>
    Entity<Idx, A, PK>::~Entity() {
        delete this->_dao;
        this->_dao = NULL;
        print("Entity destroyed \n");
    }

}
