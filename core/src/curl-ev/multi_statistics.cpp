#include <curl-ev/multi_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

MultiStatistics::MultiStatistics()
    : busy_storage_(utils::statistics::kDefaultEpochDuration,
                    utils::statistics::kDefaultMaxPeriod) {}

void MultiStatistics::mark_open_socket() { open_++; }

void MultiStatistics::mark_close_socket() { close_++; }

void MultiStatistics::mark_socket_ratelimited() { ratelimited_++; }

long long MultiStatistics::open_socket_total() const { return open_.load(); }

long long MultiStatistics::close_socket_total() const { return close_.load(); }

long long MultiStatistics::socket_ratelimited_total() const {
  return ratelimited_.load();
}

utils::statistics::BusyStorage& MultiStatistics::get_busy_storage() {
  return busy_storage_;
}

const utils::statistics::BusyStorage& MultiStatistics::get_busy_storage()
    const {
  return busy_storage_;
}

}  // namespace curl

USERVER_NAMESPACE_END
