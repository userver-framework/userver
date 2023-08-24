#include <storages/postgres/congestion_control/limiter.hpp>

#include <storages/postgres/detail/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::cc {

Limiter::Limiter(detail::ConnectionPool& pool) : pool_(pool) {}

void Limiter::SetLimit(const congestion_control::Limit& new_limit) {
  pool_.SetMaxConnectionsCc(new_limit.load_limit.value_or(0));
}

}  // namespace storages::postgres::cc

USERVER_NAMESPACE_END
