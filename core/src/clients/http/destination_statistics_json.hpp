#pragma once

#include <userver/clients/http/destination_statistics.hpp>
#include <userver/formats/json/value.hpp>

namespace clients {
namespace http {

class DestinationStatistics;

formats::json::Value DestinationStatisticsToJson(
    const DestinationStatistics& stats);

}  // namespace http
}  // namespace clients
