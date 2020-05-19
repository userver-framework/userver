#pragma once

#include <chrono>

namespace testsuite {

struct RedisControl {
  std::chrono::milliseconds min_timeout_connect =
      std::chrono::milliseconds::zero();
  std::chrono::milliseconds min_timeout_single =
      std::chrono::milliseconds::zero();
  std::chrono::milliseconds min_timeout_all = std::chrono::milliseconds::zero();
  bool disable_cluster_mode = false;
};

}  // namespace testsuite
