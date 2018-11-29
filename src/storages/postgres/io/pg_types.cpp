#include <storages/postgres/io/pg_types.hpp>

#include <boost/functional/hash.hpp>

namespace storages::postgres {

namespace {

std::size_t StrHash(const char* str, std::size_t len) {
  auto seed = len;
  boost::hash_range(seed, str, str + len);
  return seed;
}

}  // namespace

std::size_t DBTypeName::GetHash() const {
  auto seed = StrHash(schema.data(), schema.size());
  boost::hash_combine(seed, StrHash(name.data(), name.size()));
  return seed;
}

std::size_t DBTypeDescription::GetNameHash() const {
  auto seed = StrHash(schema.data(), schema.size());
  boost::hash_combine(seed, StrHash(name.data(), name.size()));
  return seed;
}

}  // namespace storages::postgres
