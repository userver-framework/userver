#include <userver/storages/postgres/detail/string_hash.hpp>

#include <boost/functional/hash.hpp>

namespace storages::postgres::utils {

std::size_t StrHash(const char* str, std::size_t len) {
  auto seed = len;
  boost::hash_range(seed, str, str + len);
  return seed;
}

}  // namespace storages::postgres::utils
