#include <userver/storages/postgres/detail/query_parameters.hpp>

#include <boost/functional/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

std::size_t QueryParameters::TypeHash() const {
  auto seed = param_types.size();
  boost::hash_range(seed, param_types.begin(), param_types.end());
  return seed;
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
