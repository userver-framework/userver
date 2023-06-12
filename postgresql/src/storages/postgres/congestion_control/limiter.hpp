#pragma once

#include <userver/congestion_control/limiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace detail {
class ConnectionPool;
}

namespace cc {

class Limiter final : public congestion_control::Limiter {
 public:
  explicit Limiter(detail::ConnectionPool& pool);

  void SetLimit(const congestion_control::Limit& new_limit) override;

 private:
  detail::ConnectionPool& pool_;
};

}  // namespace cc

}  // namespace storages::postgres

USERVER_NAMESPACE_END
