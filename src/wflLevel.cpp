#include <woffler.hpp>

uint64_t woffler::addLevel(name owner, const wflbranch& branch) {
  auto self = get_self();
  
  //find stake to use as pot value for root level
  stakes _stakes(self, self.value);

  //calculating branch stake total (all stakeholders)
  auto stkidx = _stakes.get_index<name("bybranch")>();
  auto stkitr = stkidx.lower_bound(branch.id);
  auto branchStake = asset{0, Const::acceptedSymbol};
  while(stkitr != stkidx.end()) {
    branchStake+=stkitr->stake;
    stkitr++;
  }

  //getting branch meta to decide on level presets
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(branch.idmeta);
  check(
    _meta != _metas.end(),
    "No branch metadata found for branch"
  );

  //emplacing new (root) level
  levels _levels(self, self.value);
  auto idlevel = Utils::nextPrimariKey(_levels.available_primary_key());
  auto rnd = randomizer::getInstance(owner, idlevel);
  std::vector<uint8_t> greencells = generateCells(rnd, _meta->lvlgreens, _meta->lvllength);
  std::vector<uint8_t> redcells = generateCells(rnd, _meta->lvlreds, _meta->lvllength);
  _levels.emplace(owner, [&](auto& l) {
    l.id = idlevel;
    l.idbranch = branch.id;
    l.potbalance = branchStake;
    l.redcells = redcells;
    l.greencells = greencells;
  });

  print("Root level created with id: ", std::to_string(idlevel) , ", pot balance: ", asset{branchStake}, " for branch: ", std::to_string(branch.id));    
  
  return idlevel;
}

template<class T>
std::vector<T> woffler::generateCells(randomizer& rnd, T size, T maxval) {

  std::vector<T> data(size);
  Cell::generator<T> generator(rnd, maxval, size);
  std::generate(data.begin(), data.end(), generator);

  return data;
}

void woffler::regencells(name owner, uint64_t idlevel, uint64_t idmeta) {
  require_auth(owner);
  auto self = get_self();
  check(
    owner == self,
    string("Debug mode available only to contract owner: ") + self.to_string()
  );

  //getting branch meta to decide on level presets
  brnchmetas _metas(self, self.value);    
  auto _meta = _metas.find(idmeta);
  check(
    _meta != _metas.end(),
    "No branch metadata found"
  );
  levels _levels(self, self.value);
  auto _level = _levels.find(idlevel);
  check(
    _level != _levels.end(),
    "No level found "
  );
  auto rnd = randomizer::getInstance(owner, idlevel);
  std::vector<uint8_t> greencells = generateCells(rnd, _meta->lvlgreens, _meta->lvllength);
  std::vector<uint8_t> redcells = generateCells(rnd, _meta->lvlreds, _meta->lvllength);
  _levels.modify(_level, owner, [&](auto& l) {
    l.redcells = redcells;
    l.greencells = greencells;
  });
}

void woffler::gencells(name account, uint8_t size, uint8_t maxval) {
  require_auth(account);
  auto self = get_self();
  check(
    account == self,
    string("Debug mode available only to contract owner: ") + self.to_string()
  );

  auto rnd = randomizer::getInstance(account, 1);
  auto data = generateCells<uint8_t>(rnd, size, maxval);
  Utils::printVectorInt<uint8_t>(data);
}
