#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include <boost/any.hpp>

#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>

namespace utils::statistics {

namespace impl {

template <typename Metric>
formats::json::ValueBuilder DumpMetric(const std::atomic<Metric>& m) {
  if constexpr (std::is_integral<Metric>::value ||
                std::is_floating_point<Metric>::value) {
    return m.load();
  } else {
    static_assert(false && sizeof(Metric), "std::atomic misuse");
  }
}

template <class Value, class = ::utils::void_t<>>
struct HasDumpMetric : std::false_type {};

template <class Value>
struct HasDumpMetric<
    Value, ::utils::void_t<decltype(DumpMetric(std::declval<Value&>()))>>
    : std::true_type {};

template <typename Metric>
typename std::enable_if<std::is_integral<Metric>::value,
                        formats::json::ValueBuilder>::type
DumpMetric(const Metric&) {
  static_assert(sizeof(Metric) && false,
                "Type is not atomic, use std::atomic<T> instead");
}

struct MetricInfo final {
  /* stores std::shared_ptr<T> as user type might be non-copyable (e.g.
   * std::atomic). boost::any requires copy constructor.
   */
  boost::any data_;

  std::string path;
  std::function<formats::json::ValueBuilder(boost::any&)> dump_func;
};

void RegisterMetricInfo(std::type_index ti, MetricInfo&& metric_info);

template <typename Metric>
formats::json::ValueBuilder DumpAnyMetric(boost::any& data) {
  static_assert(HasDumpMetric<Metric>::value,
                "There is no `DumpMetric(Metric& / const Metric&)` "
                "in namespace of `Metric`.  "
                "You have not provided a `DumpMetric` function overload.");
  return DumpMetric(*boost::any_cast<std::shared_ptr<Metric>&>(data));
}

template <typename Metric>
void RegisterTag(const MetricTag<Metric>& tag) {
  RegisterMetricInfo(typeid(Metric), MetricInfo{
                                         std::make_shared<Metric>(),
                                         tag.GetPath(),
                                         DumpAnyMetric<Metric>,
                                     });
}

}  // namespace impl

template <typename Metric>
MetricTag<Metric>::MetricTag(const std::string& path) : path_(path) {
  impl::RegisterTag(*this);
}

}  // namespace utils::statistics
