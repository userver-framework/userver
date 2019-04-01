#pragma once

#include <utils/statistics/busy.hpp>
#include <utils/statistics/common.hpp>

#include <atomic>

namespace curl {

class MultiStatistics {
 public:
  MultiStatistics();

  void mark_open_socket();
  void mark_close_socket();

  long long open_socket_total() const;
  long long close_socket_total() const;

  utils::statistics::BusyStorage& get_busy_storage();
  const utils::statistics::BusyStorage& get_busy_storage() const;

 private:
  std::atomic_llong open_{0};
  std::atomic_llong close_{0};
  utils::statistics::BusyStorage busy_storage_;
};

}  // namespace curl
