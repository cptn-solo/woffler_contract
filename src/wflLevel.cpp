#include <woffler.hpp>

void woffler::addLevel(name owner, 
  uint64_t idbranch, 
  asset pot,
  const wflbrnchmeta& bmeta
) {
  auto self = get_self();
  
  levels _levels(self, self.value);
  auto idlevel = Utils::nextPrimariKey(_levels.available_primary_key());
  std::vector<uint8_t> greencells = generate_data(bmeta.lvllength, bmeta.lvlreds);//TODO: fill with random numbers (1-meta::(lvllenght, lvlgreens))
  std::vector<uint8_t> redcells = generate_data(bmeta.lvllength, bmeta.lvlreds);//TODO: fill with random numbers (1-meta::(lvllenght, lvlgreens))
  _levels.emplace(owner, [&](auto& l) {
    l.id = idlevel;
    l.idbranch = idbranch;
    l.idchbranch = 0;
    l.potbalance = pot;//owner's balance cut with check (see subBalance earlier)
    l.redcells = redcells;
    l.greencells = greencells;
  });
}

std::vector<uint8_t> woffler::generate_data(uint8_t size, uint8_t maxval) {
  std::vector<uint8_t> data(size);
  cellGenerator cellPos(maxval, size);
  std::generate(data.begin(), data.end(), cellPos);

  return data;
}

void woffler::gencells(uint8_t size, uint8_t maxval) {
  auto data = generate_data(size, maxval);
  for (std::vector<uint8_t>::const_iterator i = data.begin(); i != data.end(); ++i)
    print("[", std::to_string(*i), "]");
  
}
