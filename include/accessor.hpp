#pragma once
namespace Woffler {
    template <typename Idx, typename Ent, typename Itr, typename V>
    struct Accessor {
        public:

            Accessor(Idx& idx, V val);      

            template <typename Lambda>
            void create(name payer, Lambda&& creator);
            
            template <typename Lambda>
            void update(name payer, Lambda&& updater);
            
            bool isEnt();
            const Ent& getEnt();
        
        protected:

            Idx& _idx;     
            Itr _itr;
    };

    template<typename Idx, typename Ent, typename Itr, typename V>
    Accessor<Idx, Ent, Itr, V>::Accessor(Idx& idx, V val): _itr(idx.find(val)), _idx{idx} {}

    template<typename Idx, typename Ent, typename Itr, typename V>
    template<typename Lambda>
    void Accessor<Idx, Ent, Itr, V>::create(name payer, Lambda&& creator) {
        _itr = _idx.emplace(payer, std::forward<Lambda&&>(creator)); 
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    template<typename Lambda>
    void Accessor<Idx, Ent, Itr, V>::update(name payer, Lambda&& updater) {
        _idx.modify(_itr, payer, std::forward<Lambda&&>(updater)); 
    }
    template<typename Idx, typename Ent, typename Itr, typename V>
    bool Accessor<Idx, Ent, Itr, V>::isEnt() {      
        return _itr != _idx.end();
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    const Ent& Accessor<Idx, Ent, Itr, V>::getEnt() {
        check(isEnt(), "Object not found.");
        return *_itr;
    }            



}
