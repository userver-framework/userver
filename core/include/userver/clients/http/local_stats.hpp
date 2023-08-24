#pragma once

#include <chrono>
#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// Represents all the local timings and statistics
struct LocalStats final {
  using duration = std::chrono::steady_clock::time_point::duration;

  duration time_to_connect{};

  /// total time
  duration time_to_process{};

  size_t open_socket_count = 0;

  /// returns 0 based retires count. In other words:
  ///   0 - the very first request succeeded
  ///   1 - made 1 retry
  ///   2 - made 2 retries
  ///   ...
  ///
  size_t retries_count = 0;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
