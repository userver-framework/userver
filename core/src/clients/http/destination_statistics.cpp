#include <clients/http/destination_statistics.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

struct FullInstanceStatisticsView {
  const InstanceStatistics& data;
};

void DumpMetric(utils::statistics::Writer& writer,
                FullInstanceStatisticsView stats) {
  DumpMetric(writer, stats.data, FormatMode::kModeDestination);
}

}  // namespace

std::shared_ptr<RequestStats>
DestinationStatistics::GetStatisticsForDestination(
    const std::string& destination) {
  auto ptr = GetExistingStatisticsForDestination(destination);
  if (ptr) return ptr;

  return CreateStatisticsForDestination(destination);
}

std::shared_ptr<RequestStats>
DestinationStatistics::CreateStatisticsForDestination(
    const std::string& destination) {
  return std::make_shared<RequestStats>(*rcu_map_[destination]);
}

std::shared_ptr<RequestStats>
DestinationStatistics::GetExistingStatisticsForDestination(
    const std::string& destination) {
  /*
   * It's safe to return RequestStats holding a reference to Statistics as
   * RequestStats lifetime is less than clients::http::Client's one.
   */
  auto stats = rcu_map_.Get(destination);
  if (stats)
    return std::make_shared<RequestStats>(*stats);
  else
    return {};
}

std::shared_ptr<RequestStats>
DestinationStatistics::GetStatisticsForDestinationAuto(
    const std::string& destination) {
  auto ptr = GetExistingStatisticsForDestination(destination);
  if (ptr) return ptr;

  // atomic [current++ iff current < max]
  auto current_auto_destinations = current_auto_destinations_.load();
  do {
    if (current_auto_destinations >= max_auto_destinations_) {
      if (max_auto_destinations_ != 0) {
        LOG_LIMITED_WARNING()
            << "Too many httpclient metrics destinations used ("
            << max_auto_destinations_
            << "), either increase "
               "components.http-client.destination-metrics-auto-max-size "
               "or explicitly set destination via "
               "Request::SetDestinationMetricName().";
      }
      return {};
    }
  } while (!current_auto_destinations_.compare_exchange_strong(
      current_auto_destinations, current_auto_destinations + 1));

  return CreateStatisticsForDestination(destination);
}

void DestinationStatistics::SetAutoMaxSize(size_t max_auto_destinations) {
  max_auto_destinations_ = max_auto_destinations;
}

DestinationStatistics::DestinationsMap::ConstIterator
DestinationStatistics::begin() const {
  return rcu_map_.begin();
}

DestinationStatistics::DestinationsMap::ConstIterator
DestinationStatistics::end() const {
  return rcu_map_.end();
}

void DumpMetric(utils::statistics::Writer& writer,
                const DestinationStatistics& stats) {
  for (const auto& [url, stat_ptr] : stats) {
    const InstanceStatistics instance_stat{*stat_ptr};
    writer.ValueWithLabels(FullInstanceStatisticsView{instance_stat},
                           {"http_destination", url});
  }
}

}  // namespace clients::http

USERVER_NAMESPACE_END
