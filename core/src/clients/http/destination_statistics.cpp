#include <clients/http/destination_statistics.hpp>

namespace clients {
namespace http {

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

  // create new item
  if (current_auto_destinations_++ > max_auto_destinations_) {
    current_auto_destinations_--;
    // TODO: LOG_ERROR() and ratelimit
    LOG_WARNING() << "Too many httpclient metrics destinations used ("
                  << max_auto_destinations_
                  << "), either increase "
                     "components.http-client.destination-metrics-auto-max-size "
                     "or explicitly set destination via "
                     "Request::SetDestinationMetricName().";
    return {};
  }

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

}  // namespace http
}  // namespace clients
