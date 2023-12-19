#include <storages/mongo/congestion_control/limiter.hpp>

#include <storages/mongo/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::cc {

Limiter::Limiter(impl::PoolImpl& pool) : pool_(pool) {}

void Limiter::SetLimit(const congestion_control::Limit& new_limit) {
  if (init_max_size_ == 0) init_max_size_ = pool_.MaxSize();

  auto max_size = new_limit.load_limit.value_or(init_max_size_);
  if (max_size > init_max_size_) max_size = init_max_size_;

  pool_.SetMaxSize(max_size);
}

}  // namespace storages::mongo::cc

USERVER_NAMESPACE_END
