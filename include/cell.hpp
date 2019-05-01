#include <rand.hpp>
namespace Cell {
  template <typename T>
  struct generator {
    T _current;
    T _step;
    randomizer _randomGen;
    generator(randomizer& rnd, T maxval, T size) {
      _randomGen = rnd;
      _current = 0;
      _step = (maxval / size);
    }
    T operator()() {  
      //get random for each segment of length == step 
      T _retval = (
        _current == 0 ? 
          (_randomGen.range(_step-1)+1) : /* exclude 0 position for player "respawn" point */
          (_randomGen.range(_step)+_current)
      );

      _current += _step;

      return _retval;
    }
  };
  
}  
