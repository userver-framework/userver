#pragma once

#include <optional>
#include <vector>

#include <userver/concurrent/variable.hpp>
#include <userver/congestion_control/limiter.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

class Limitee {
 public:
  virtual void SetLimit(std::optional<size_t> new_limit) = 0;
};

class Limiter final : public USERVER_NAMESPACE::congestion_control::Limiter {
 public:
  void SetLimit(
      const USERVER_NAMESPACE::congestion_control::Limit& new_limit) override;

  void RegisterLimitee(Limitee& limitee);

 private:
  concurrent::Variable<std::vector<utils::NotNull<Limitee*>>, std::mutex>
      limitees_;
};

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
