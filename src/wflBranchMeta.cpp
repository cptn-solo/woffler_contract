#include <woffler.hpp>

//*** Branch scope methods (wflBranch) ***//

void woffler::cleanbrmeta(name owner, uint64_t idmeta) {
  require_auth(owner);
  print("Deleting branch meta: ", std::to_string(idmeta));

  auto self = get_self();
  
  brnchmetas _metas(self, self.value);
  auto _meta = _metas.find(idmeta);
  check(
    _meta != _metas.end(),
    "No branch metadata found for id"
  );
  check(
    !_meta->owner || _meta->owner == owner,
    "Branch metadata can be deleted only by its owner"
  );
  
  //TODO: check if there are branches using meta being deleted

  _metas.erase(_meta);
  
  print("Branch metadata Removed");

}

void woffler::brnchmeta(name owner, 
  uint64_t id,  
  asset unjlmin,
  uint8_t unjlrate,
  uint64_t unjlintrvl,
  uint8_t tkrate,
  uint64_t tkintrvl,
  uint8_t nxtrate,
  uint8_t spltrate,
  asset stkmin,
  uint8_t stkrate,
  asset potmin,
  uint8_t slsrate,
  string url,
  string name
) {
  require_auth(owner);
  print("Create/update branch meta: ", name);

  auto self = get_self();
  
  brnchmetas _metas(self, self.value);
    
  if (id >= 1) {
    auto _meta = _metas.find(id);
    check(
      _meta != _metas.end(),
      "No branch metadata found for id"
    );

    check(
      _meta->owner == owner,
      "Branch metadata can be modified only by its owner"
    );

    _metas.modify(_meta, owner, [&](auto& m) {
      m.unjlrate = unjlrate;
      m.unjlintrvl = unjlintrvl;
      m.tkrate = tkrate;
      m.tkintrvl = tkintrvl;
      m.nxtrate = nxtrate;
      m.spltrate = spltrate;
      m.stkmin = stkmin;
      m.stkrate = stkrate;
      m.potmin = potmin;
      m.slsrate = slsrate;
      m.url = url;
      m.name = name;
    });
  } 
  else {
    _metas.emplace(owner, [&](auto& m) {
      m.id = Utils::nextPrimariKey(_metas.available_primary_key());
      m.owner = owner;
      m.unjlrate = unjlrate;
      m.unjlintrvl = unjlintrvl;
      m.tkrate = tkrate;
      m.tkintrvl = tkintrvl;
      m.nxtrate = nxtrate;
      m.spltrate = spltrate;
      m.stkmin = stkmin;
      m.stkrate = stkrate;
      m.potmin = potmin;
      m.slsrate = slsrate;
      m.url = url;
      m.name = name;
    });
  }
}
