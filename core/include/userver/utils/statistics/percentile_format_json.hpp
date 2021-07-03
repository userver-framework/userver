#pragma once

#include <initializer_list>

#include <formats/json/value_builder.hpp>
#include <utils/assert.hpp>
#include <utils/statistics/metadata.hpp>
#include <utils/statistics/percentile.hpp>

namespace utils {
namespace statistics {

std::string GetPercentileFieldName(double perc);

template <size_t M, typename T, size_t N, size_t K>
formats::json::ValueBuilder PercentileToJson(
    const Percentile<M, T, N, K>& perc,
    std::initializer_list<double> percents) {
  formats::json::ValueBuilder result;
  for (double percent : percents) {
    result[GetPercentileFieldName(percent)] = perc.GetPercentile(percent);
  }
  utils::statistics::SolomonChildrenAreLabelValues(result, "percentile");
  return result;
}

template <size_t M, typename T, size_t N, size_t K>
formats::json::ValueBuilder PercentileToJson(
    const Percentile<M, T, N, K>& perc) {
  return PercentileToJson(perc, {0, 50, 90, 95, 98, 99, 99.6, 99.9, 100});
}

}  // namespace statistics
}  // namespace utils
