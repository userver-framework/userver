#pragma once

#include <userver/formats/json/value.hpp>

#include <userver/kafka/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka {

formats::json::Value ExtendStatistics(const Stats& stats);

}  // namespace kafka

USERVER_NAMESPACE_END
