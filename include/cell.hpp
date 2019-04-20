namespace Cell {
  template <typename T>
  struct generator {
    T _current;
    T _step;
    generator(T maxval, T size) {
        _current = 0;
        _step = (maxval / size);
    }
    T operator()() {  
        _current += _step;
        return _current;
    }
  };
  
}