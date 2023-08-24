#include <userver/storages/postgres/detail/query_parameters.hpp>

#include <boost/functional/hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

std::size_t QueryParameters::TypeHash() const {
  auto seed = size_;
  boost::hash_range(seed, types_, types_ + size_);
  return seed;
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
