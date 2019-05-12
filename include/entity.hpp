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
    void fetchByKey(PK entKey);//fetch entity by key when instantiating without initially known key

    template <typename Lambda>
    void create(name payer, Lambda&& creator);

    template <typename Lambda>
    void update(name payer, Lambda&& updater);

    void remove();

    bool isEnt();
    bool isEnt(PK val);

    template <typename Ent>
    const Ent& getEnt();

    PK nextPK();

    template<name::raw IndexName>
    auto getIndex();
    
    name _self;
    PK _entKey;
    Idx _idx;

    private:
    A* _dao = NULL;
  };

  template<typename Idx, typename A, typename PK>
  Entity<Idx, A, PK>::Entity(name self, PK entKey): _idx(self, self.value) {
    _self = self;
    _entKey = entKey;
    if (A::keyValue(entKey) > 0)
      _dao = new A(_idx, A::keyValue(entKey));
  }

  template<typename Idx, typename A, typename PK>
  Entity<Idx, A, PK>::~Entity() {
    if (_dao) {
      delete _dao;
      _dao = NULL;
    }
    print("Entity destroyed \n");
  }

  template<typename Idx, typename A, typename PK>
  void Entity<Idx, A, PK>::fetchByKey(PK entKey) {
    check(_dao == NULL, "Can't refetch into existing accessor object");
    _dao = new A(_idx, A::keyValue(entKey));
  }

  template<typename Idx, typename A, typename PK>
  template <typename Lambda>
  void Entity<Idx, A, PK>::create(name payer, Lambda&& creator) {
    auto _itr = _idx.emplace(payer, std::forward<Lambda&&>(creator));
    _dao = new A(_idx, _itr);
  }

  template<typename Idx, typename A, typename PK>
  template <typename Lambda>
  void Entity<Idx, A, PK>::update(name payer, Lambda&& updater) {
    check(isEnt(), "Object not found.");
    _dao->update(payer, updater);
  }

  template<typename Idx, typename A, typename PK>
  void Entity<Idx, A, PK>::remove() {
    check(isEnt(), "Object not found.");
    _dao->remove();
  }

  template<typename Idx, typename A, typename PK>
  bool Entity<Idx, A, PK>::isEnt() {
    if (!_dao) return false;
    
    return _dao->isEnt();    
  }

  template<typename Idx, typename A, typename PK>
  bool Entity<Idx, A, PK>::isEnt(PK val) {
    if (!_dao) return false;
    
    return _dao->isEnt(A::keyValue(val));
  }

  template<typename Idx, typename A, typename PK>
  template<typename Ent>
  const Ent& Entity<Idx, A, PK>::getEnt() {
    check(isEnt(), "Object not found.");
    return _dao->getEnt();
  }

  template<typename Idx, typename A, typename PK>
  PK Entity<Idx, A, PK>::nextPK() {
    return Utils::nextPrimariKey(_idx.available_primary_key());
  }
  
  template<typename Idx, typename A, typename PK>
  template<name::raw IndexName>
  auto Entity<Idx, A, PK>::getIndex() {
    auto idx = _idx.template get_index<name(IndexName)>();
    return idx;
  }
}
