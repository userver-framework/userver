#include <userver/storages/postgres/io/pg_types.hpp>

#include <boost/functional/hash.hpp>

#include <userver/storages/postgres/detail/string_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

std::size_t DBTypeName::GetHash() const {
  auto seed = utils::StrHash(schema);
  boost::hash_combine(seed, utils::StrHash(name));
  return seed;
}

std::string DBTypeName::ToString() const {
  std::string res{schema};
  res += "." + std::string{name};
  return res;
}

std::size_t DBTypeDescription::GetNameHash() const {
  auto seed = utils::StrHash(schema);
  boost::hash_combine(seed, utils::StrHash(name));
  return seed;
}

std::string ToString(DBTypeDescription::TypeClass c) {
  using TypeClass = DBTypeDescription::TypeClass;
  switch (c) {
    case TypeClass::kUnknown:
      return "unknown";
    case TypeClass::kBase:
      return "base";
    case TypeClass::kComposite:
      return "composite";
    case TypeClass::kDomain:
      return "domain";
    case TypeClass::kEnum:
      return "enumeration";
    case TypeClass::kPseudo:
      return "pseudo";
    case TypeClass::kRange:
      return "range";
  }
  return "unknown";
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
