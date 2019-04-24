#include <clients/http/destination_statistics_json.hpp>

#include <clients/http/destination_statistics.hpp>
#include <formats/json/value_builder.hpp>

namespace clients {
namespace http {

formats::json::Value DestinationStatisticsToJson(
    const DestinationStatistics& stats) {
  formats::json::ValueBuilder json;
  for (const auto& [url, stat_ptr] : stats)
    json[url] = StatisticsToJson(InstanceStatistics(*stat_ptr));
  return json.ExtractValue();
}

}  // namespace http
}  // namespace clients
