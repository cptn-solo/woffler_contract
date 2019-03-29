#include "woffler.hpp"

ACTION woffler::hi(name user) {
  require_auth(user);
  print("Hello, ", name{user});
}
