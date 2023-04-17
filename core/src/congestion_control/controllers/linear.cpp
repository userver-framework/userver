#include <userver/congestion_control/controllers/linear.hpp>

#include <userver/utils/impl/userver_experiments.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control::v2 {

namespace {
constexpr double kTimeoutRateThreshold = 0.05;  // 5%
constexpr size_t kSafeDeltaLimit = 10;
}  // namespace

Limit LinearController::Update(Sensor::Data& current) {
  auto rate = current.GetRate();
  if (rate > kTimeoutRateThreshold) {
    if (current_limit_) {
      --*current_limit_;
    } else {
      LOG_ERROR() << "Congestion Control is activated";
      current_limit_ = current.current_limit;  // TODO: * 0.7 or smth
    }
    LOG_ERROR() << "Congestion Control is active, connpool max size "
                   "is incremented to "
                << current_limit_;
  } else {
    if (current_limit_) {
      if (current_limit_ > current.current_limit + kSafeDeltaLimit) {
        current_limit_.reset();
      } else {
        ++*current_limit_;
      }
    }
    LOG_ERROR() << "Postgres Congestion Control is active, connpool max size "
                   "is decremented to "
                << current_limit_;
  }

  return {current_limit_, current.current_limit};
}

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
