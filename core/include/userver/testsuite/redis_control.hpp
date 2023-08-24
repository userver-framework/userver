#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

struct RedisControl {
  std::chrono::milliseconds min_timeout_connect =
      std::chrono::milliseconds::zero();
  std::chrono::milliseconds min_timeout_single =
      std::chrono::milliseconds::zero();
  std::chrono::milliseconds min_timeout_all = std::chrono::milliseconds::zero();
};

}  // namespace testsuite

USERVER_NAMESPACE_END
