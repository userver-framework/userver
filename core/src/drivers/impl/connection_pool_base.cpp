#include <userver/drivers/impl/connection_pool_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace drivers::impl {

PoolWaitLimitExceededError::PoolWaitLimitExceededError()
    : std::runtime_error{"Connection pool acquisition wait limit exceeded"} {}

}  // namespace drivers::impl

USERVER_NAMESPACE_END
