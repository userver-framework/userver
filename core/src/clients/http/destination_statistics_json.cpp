#include <clients/http/destination_statistics_json.hpp>

#include <clients/http/destination_statistics.hpp>
#include <formats/json/value_builder.hpp>

namespace clients {
namespace http {

formats::json::Value DestinationStatisticsToJson(
    const DestinationStatistics::DestinationsMap& stats) {
  formats::json::ValueBuilder json;
  for (const auto& it : stats)
    json[it.first] = StatisticsToJson(InstanceStatistics(*it.second));
  return json.ExtractValue();
}

}  // namespace http
}  // namespace clients
