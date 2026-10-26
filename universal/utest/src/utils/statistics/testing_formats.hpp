#pragma once

#include <userver/formats/json_fwd.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

/// JSON object in the shape produced by ToJsonFormat.
struct FlatJson final {
    const formats::json::Value& json;
};

void DumpMetric(Writer& writer, const FlatJson& metrics);

/// Layered JSON object. Top-level keys are paths, nested `"name = value"` keys are labels.
struct LayeredJson final {
    const formats::json::Value& json;
};

void DumpMetric(Writer& writer, const LayeredJson& metrics);

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
