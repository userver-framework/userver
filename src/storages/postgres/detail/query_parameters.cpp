#include <storages/postgres/detail/query_parameters.hpp>

#include <boost/functional/hash.hpp>

namespace storages {
namespace postgres {
namespace detail {

std::size_t QueryParameters::TypeHash() const {
  auto seed = param_types.size();
  boost::hash_range(seed, param_types.begin(), param_types.end());
  return seed;
}

}  // namespace detail
}  // namespace postgres
}  // namespace storages
