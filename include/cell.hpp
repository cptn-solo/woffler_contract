namespace Cell {
  struct generator {
    uint8_t _current;
    uint8_t _step;
    generator(uint8_t maxval, uint8_t size) {
        _current = 0;
        _step = ((maxval - 1) / size);
    }
    uint8_t operator()() {  
        _current += _step;
        return _current;
    }
  };
  
}