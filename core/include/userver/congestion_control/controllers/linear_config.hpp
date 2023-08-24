#pragma once

#include <chrono>
#include <cstddef>

#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json {
class Value;
}

namespace congestion_control::v2 {

struct Config {
  Config() = default;

  explicit Config(formats::json::Value config);

  double errors_threshold_percent{5.0};  // 5%
  std::size_t safe_delta_limit{10};
  std::size_t timings_burst_threshold{5};
  std::chrono::milliseconds min_timings{20};
  std::size_t min_limit{10};
  std::size_t min_qps{10};
};

}  // namespace congestion_control::v2

USERVER_NAMESPACE_END
