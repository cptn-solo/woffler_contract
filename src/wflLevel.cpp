#include <woffler.hpp>

void woffler::addLevel(name owner, 
  uint64_t idbranch, 
  asset pot,
  const wflbrnchmeta& bmeta
) {
  auto self = get_self();
  
  levels _levels(self, self.value);
  auto idlevel = Utils::nextPrimariKey(_levels.available_primary_key());
  std::vector<uint8_t> greencells = generateCells(owner, bmeta.lvllength, bmeta.lvlreds);//TODO: fill with random numbers (1-meta::(lvllenght, lvlgreens))
  std::vector<uint8_t> redcells = generateCells(owner, bmeta.lvllength, bmeta.lvlreds);//TODO: fill with random numbers (1-meta::(lvllenght, lvlgreens))
  _levels.emplace(owner, [&](auto& l) {
    l.id = idlevel;
    l.idbranch = idbranch;
    l.idchbranch = 0;
    l.potbalance = pot;//owner's balance cut with check (see subBalance in wflBranch.cpp->woffler::branch)
    l.redcells = redcells;
    l.greencells = greencells;
  });
}

template<class T>
std::vector<T> woffler::generateCells(name account, T size, T maxval) {

  std::vector<T> data(size);
  auto rnd = randomizer::getInstance(account);
  Cell::generator<T> generator(rnd, maxval, size);
  std::generate(data.begin(), data.end(), generator);

  return data;
}

void woffler::gencells(name account, uint8_t size, uint8_t maxval) {
  auto data = generateCells<uint8_t>(account, size, maxval);
  Utils::printVectorInt<uint8_t>(data);
}
