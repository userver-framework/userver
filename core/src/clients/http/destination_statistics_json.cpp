#include <clients/http/destination_statistics_json.hpp>

#include <clients/http/destination_statistics.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

formats::json::Value DestinationStatisticsToJson(
    const DestinationStatistics& stats) {
  formats::json::ValueBuilder json;
  for (const auto& [url, stat_ptr] : stats)
    json[url] = StatisticsToJson(InstanceStatistics(*stat_ptr),
                                 FormatMode::kModeDestination);
  return json.ExtractValue();
}

}  // namespace clients::http

USERVER_NAMESPACE_END
