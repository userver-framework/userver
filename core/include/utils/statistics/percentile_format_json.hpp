#pragma once

#include <utils/statistics/percentile.hpp>

namespace utils {
namespace statistics {

template <size_t M, typename T, size_t N, size_t K>
formats::json::ValueBuilder PercentileToJson(
    const Percentile<M, T, N, K>& perc) {
  static const size_t percents[] = {0, 50, 90, 95, 98, 99, 100};

  formats::json::ValueBuilder result(formats::json::Type::kObject);
  for (const auto& percent : percents) {
    result["p" + std::to_string(percent)] = perc.GetPercentile(percent);
  }

  result["p99_6"] = perc.GetPercentile(99.6);
  result["p99_9"] = perc.GetPercentile(99.9);
  return result;
}

}  // namespace statistics
}  // namespace utils
