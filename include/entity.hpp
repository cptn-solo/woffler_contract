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
            template <typename Lambda>
            void create(name payer, Lambda&& creator);
            
            template <typename Lambda>
            void update(name payer, Lambda&& updater);
            
            void remove();
                        
            bool isEnt();
            bool isEnt(PK val);
            
            template <typename Ent>
            const Ent& getEnt();

            name _self;
            PK _entKey;            

        private:
            A* _dao = NULL;
            Idx _idx;     
    };

    template<typename Idx, typename A, typename PK>
    Entity<Idx, A, PK>::Entity(name self, PK entKey): _idx(self, self.value) {
        this->_self = self;
        this->_entKey = entKey;
        this->_dao = new A(_idx, A::keyValue(entKey));
    }

    template<typename Idx, typename A, typename PK>
    Entity<Idx, A, PK>::~Entity() {
        delete this->_dao;
        this->_dao = NULL;
        print("Entity destroyed \n");
    }

    template<typename Idx, typename A, typename PK>
    template <typename Lambda>
    void Entity<Idx, A, PK>::create(name payer, Lambda&& creator) {
        _dao->create(payer, creator);
    }
    
    template<typename Idx, typename A, typename PK>
    template <typename Lambda>
    void Entity<Idx, A, PK>::update(name payer, Lambda&& updater) {
        _dao->update(payer, updater);
    }
    
    template<typename Idx, typename A, typename PK>
    void Entity<Idx, A, PK>::remove() {
        _dao->remove();
    }
    
    template<typename Idx, typename A, typename PK>
    bool Entity<Idx, A, PK>::isEnt() {
        return _dao->isEnt();
    }
    
    template<typename Idx, typename A, typename PK>
    bool Entity<Idx, A, PK>::isEnt(PK val) {
        return _dao->isEnt(A::keyValue(val));
    }
    
    template<typename Idx, typename A, typename PK>
    template<typename Ent>
    const Ent& Entity<Idx, A, PK>::getEnt() {
        return _dao->getEnt();
    }
}
