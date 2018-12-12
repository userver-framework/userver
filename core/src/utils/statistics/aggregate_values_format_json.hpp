#pragma once

#include <formats/json/value_builder.hpp>
#include <utils/statistics.hpp>

namespace utils {
namespace statistics {

template <size_t length>
formats::json::Value AggregatedValuesToJson(
    const ::statistics::AggregatedValues<length>& stats,
    const std::string& suffix = {}) {
  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (size_t i = 0; i < length; ++i) {
    auto t1 = std::to_string(i ? (1 << i) : 0);
    auto t2 = (i == length - 1) ? "x" : std::to_string((1 << (i + 1)) - 1);
    std::string key = t1 + "-" + t2 + suffix;
    result[key] = stats.value[i].load();
  }
  return result.ExtractValue();
}

}  // namespace statistics
}  // namespace utils
