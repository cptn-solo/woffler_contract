#pragma once
namespace Woffler {
    template <typename Idx, typename Ent, typename Itr, typename V>
    class Accessor {
        public:

            Accessor(Idx& idx, V val);  
            ~Accessor();

            template <typename Lambda>
            void create(name payer, Lambda&& creator);
            
            template <typename Lambda>
            void update(name payer, Lambda&& updater);
            
            void remove();
                        
            bool isEnt();
            bool isEnt(V val);
            const Ent& getEnt();
        
        protected:
            Ent _ent;

        private:
            void save(name payer);
            Idx& _idx;     
            Itr _itr;
    };

    template<typename Idx, typename Ent, typename Itr, typename V>
    Accessor<Idx, Ent, Itr, V>::Accessor(Idx& idx, V val): _itr(idx.find(val)), _idx{idx} {
        if (isEnt())
            _ent = *_itr;
    }
    template<typename Idx, typename Ent, typename Itr, typename V>
    Accessor<Idx, Ent, Itr, V>::~Accessor() {
        print("Accessor destroyed \n");
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    template<typename Lambda>
    void Accessor<Idx, Ent, Itr, V>::create(name payer, Lambda&& creator) {
        _itr = _idx.emplace(payer, std::forward<Lambda&&>(creator)); 
        _ent = *_itr;
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    template<typename Lambda>
    void Accessor<Idx, Ent, Itr, V>::update(name payer, Lambda&& updater) {
        //_idx.modify(_ent, payer, std::forward<Lambda&&>(updater)); 
        updater(_ent);
        save(payer);
    }
    template<typename Idx, typename Ent, typename Itr, typename V>
    void Accessor<Idx, Ent, Itr, V>::save(name payer) {
        _idx.modify(_itr, payer, [&](auto& e) {
            e = _ent;
        });
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    void Accessor<Idx, Ent, Itr, V>::remove() {
        _idx.erase(_itr);
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    bool Accessor<Idx, Ent, Itr, V>::isEnt() {      
        return _itr != _idx.end();
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    bool Accessor<Idx, Ent, Itr, V>::isEnt(V val) {      
        return _idx.find(val) != _idx.end();
    }

    template<typename Idx, typename Ent, typename Itr, typename V>
    const Ent& Accessor<Idx, Ent, Itr, V>::getEnt() {
        check(isEnt(), "Object not found.");
        return *_itr;
    }            



}
