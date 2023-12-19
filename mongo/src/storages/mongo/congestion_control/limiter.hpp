#pragma once

#include <userver/congestion_control/limiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

namespace impl {
class PoolImpl;
}

namespace cc {

class Limiter final : public congestion_control::Limiter {
 public:
  explicit Limiter(impl::PoolImpl& pool);

  void SetLimit(const congestion_control::Limit& new_limit) override;

 private:
  impl::PoolImpl& pool_;
  std::size_t init_max_size_{0};
};

}  // namespace cc

}  // namespace storages::mongo

USERVER_NAMESPACE_END
