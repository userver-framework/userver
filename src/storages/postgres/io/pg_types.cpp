#include <storages/postgres/io/pg_types.hpp>

#include <boost/functional/hash.hpp>

#include <storages/postgres/detail/string_hash.hpp>

namespace storages::postgres {

std::size_t DBTypeName::GetHash() const {
  auto seed = utils::StrHash(schema);
  boost::hash_combine(seed, utils::StrHash(name));
  return seed;
}

std::size_t DBTypeDescription::GetNameHash() const {
  auto seed = utils::StrHash(schema);
  boost::hash_combine(seed, utils::StrHash(name));
  return seed;
}

}  // namespace storages::postgres
