#pragma once
#include <utils.hpp>
#include <constants.hpp>
#include <accessor.hpp>

namespace Woffler {
    using namespace eosio;
    using std::string;

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
        
        A d(_idx, A::keyValue(entKey));
        this->_dao = &d;
    }

    template<typename Idx, typename A, typename PK>
    Entity<Idx, A, PK>::~Entity() {
        delete this->_dao;
        this->_dao = NULL;
    }

}
