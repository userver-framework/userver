#pragma once

#include <string>
#include <vector>

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void SetSubField(formats::json::ValueBuilder& object,
                 std::vector<std::string>&& path,
                 formats::json::ValueBuilder&& value);

std::optional<std::string> FindNonNumberMetricPath(
    const formats::json::Value& json);

bool AreAllMetricsNumbers(const formats::json::Value& json);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
