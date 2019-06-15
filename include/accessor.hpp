#pragma once
namespace Woffler {
  template <typename Idx, typename Ent, typename Itr, typename V>
  class Accessor {

    private:
    
    Itr _itr;

    public:
    
    Accessor(Idx& idx, V val): Accessor(idx, idx.find(val)) {}
    Accessor(Idx& idx, Itr itr): _itr(itr), _idx{idx} {}
    ~Accessor() {}

    void fetchByKey(V entKey) {
      _itr = _idx.find(entKey);
      check(_itr != _idx.end(), "Object for key was not found in index");
    }

    template<typename I, typename A, typename PK, typename E>
    friend class Entity;

    protected:    

    Idx& _idx;

    template<typename Lambda>
    void update(name payer, Lambda&& updater) {
      _idx.modify(_itr, payer, std::forward<Lambda&&>(updater));
    }

    void remove() {
      _idx.erase(_itr);
    }

    bool isEnt() {
      return _itr != _idx.end();
    }

    bool isEnt(V val) {
      return _idx.find(val) != _idx.end();
    }

    const Ent& getEnt() {
      return *_itr;
    }        
  }; 
}
