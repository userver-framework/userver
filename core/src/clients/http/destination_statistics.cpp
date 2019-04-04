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
  auto writer = rcu_map_.StartWrite();

  // Recheck whether there's still no destination - other thread might already
  // insert between ->find() and .StartWrite()
  if (writer->count(destination) == 0)
    (*writer)[destination] = std::make_shared<Statistics>();
  writer.Commit();

  // Re-read map with inserted value
  return std::make_shared<RequestStats>(*(*rcu_map_.Read()).at(destination));
}

std::shared_ptr<RequestStats>
DestinationStatistics::GetExistingStatisticsForDestination(
    const std::string& destination) {
  /*
   * It's safe to return RequestStats holding a reference to Statistics as
   * RequestStats lifetime is less than clients::http::Client's one.
   */
  auto reader = rcu_map_.Read();
  auto it = reader->find(destination);
  if (it != reader->end())
    return std::make_shared<RequestStats>(*it->second);
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

DestinationStatistics::DestinationsMap DestinationStatistics::GetCurrentMap()
    const {
  auto ptr = rcu_map_.Read();
  return *ptr;
}

}  // namespace http
}  // namespace clients
