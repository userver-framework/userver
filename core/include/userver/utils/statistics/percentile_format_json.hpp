#pragma once

#include <initializer_list>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

template <typename T>
using HasGetPercentileResult =
    decltype(std::declval<const T&>().GetPercentile(0.0));

}  // namespace impl

std::string GetPercentileFieldName(double perc);

template <typename T>
formats::json::ValueBuilder PercentileToJson(
    const T& perc, std::initializer_list<double> percents) {
  static_assert(meta::kIsDetected<impl::HasGetPercentileResult, T>,
                "T must specify T::GetPercentile(double) returning "
                "json-serializable value");
  formats::json::ValueBuilder result;
  for (double percent : percents) {
    result[GetPercentileFieldName(percent)] = perc.GetPercentile(percent);
  }
  utils::statistics::SolomonChildrenAreLabelValues(result, "percentile");
  return result;
}

template <typename T>
formats::json::ValueBuilder PercentileToJson(const T& perc) {
  return PercentileToJson(perc, {0, 50, 90, 95, 98, 99, 99.6, 99.9, 100});
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
