#pragma once
#include <utils.hpp>
#include <constants.hpp>
#include <accessor.hpp>

namespace Woffler {
  using namespace eosio;
  using std::string;

  //types of multi-index, accessor and an entity being indexed and accessed
  template<typename Idx, typename A, typename PK, typename Ent>
  class Entity {

    private:
    
    A* _dao = NULL;

    public:

    Entity(const name& self, const PK& entKey): _idx(self, self.value) {
      _self = self;
      _entKey = entKey;
      if (A::keyValue(entKey) > 0)
        _dao = new A(_idx, A::keyValue(entKey));
    }
    ~Entity() {
      if (_dao) {
        delete _dao;
        _dao = NULL;
      }
    }

    void fetchByKey(const PK& entKey) {//fetch entity by key when instantiating without initially known key
      check(_dao == NULL, "Can't refetch into existing accessor object");
      _entKey = entKey;
      _dao = new A(_idx, A::keyValue(entKey));
    }

    protected:

    name _self;
    PK _entKey;
    Idx _idx;

    template <typename Lambda>
    const Ent& create(const name& payer, Lambda&& creator) {
      auto _itr = _idx.emplace(payer, std::forward<Lambda&&>(creator));
      _dao = new A(_idx, _itr);
      return *_itr;
    }

    template <typename Lambda>
    const Ent& update(const name& payer, Lambda&& updater) {
      check(isEnt(), "Object not found.");
      _dao->update(payer, updater);
      return _dao->getEnt();
    }

    void remove() {
      check(isEnt(), "Object not found.");
      _dao->remove();
    }

    bool isEnt() {
      return _dao ? _dao->isEnt() : false;
    }

    bool isEnt(const PK& val) {
      return _dao ? _dao->isEnt(A::keyValue(val)) : false;
    }

    const Ent& getEnt() {
      check(isEnt(), "Object not found.");
      return _dao->getEnt();
    }

    PK nextPK() {
      return Utils::nextPrimariKey(_idx.available_primary_key());
    }
    
    template<name::raw IndexName>
    auto getIndex() {
      auto idx = _idx.template get_index<name(IndexName)>();
      return idx;
    }
  };  
}
