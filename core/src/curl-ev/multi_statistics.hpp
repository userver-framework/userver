#pragma once

#include <userver/utils/statistics/busy.hpp>
#include <userver/utils/statistics/common.hpp>

#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace curl {

class MultiStatistics final {
 public:
  MultiStatistics();

  void mark_open_socket();
  void mark_close_socket();
  void mark_socket_ratelimited();

  long long open_socket_total() const;
  long long close_socket_total() const;
  long long socket_ratelimited_total() const;

  utils::statistics::BusyStorage& get_busy_storage();
  const utils::statistics::BusyStorage& get_busy_storage() const;

 private:
  std::atomic_llong open_{0};
  std::atomic_llong close_{0};
  std::atomic_llong ratelimited_{0};
  utils::statistics::BusyStorage busy_storage_;
};

}  // namespace curl

USERVER_NAMESPACE_END
