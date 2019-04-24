#pragma once

#include <clients/http/destination_statistics.hpp>
#include <formats/json/value.hpp>

namespace clients {
namespace http {

class DestinationStatistics;

formats::json::Value DestinationStatisticsToJson(
    const DestinationStatistics& stats);

}  // namespace http
}  // namespace clients
