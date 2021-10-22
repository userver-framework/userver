#pragma once

#include <any>
#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

template <typename Metric>
formats::json::ValueBuilder DumpMetric(const std::atomic<Metric>& m) {
  if constexpr (std::is_integral<Metric>::value ||
                std::is_floating_point<Metric>::value) {
    return m.load();
  } else {
    static_assert(!sizeof(Metric), "std::atomic misuse");
  }
}

template <typename Metric>
void ResetMetric(std::atomic<Metric>& m) {
  if constexpr (std::is_integral<Metric>::value ||
                std::is_floating_point<Metric>::value) {
    m = 0;
  } else {
    static_assert(!sizeof(Metric), "std::atomic misuse");
  }
}

template <class Value, class = utils::void_t<>>
struct HasDumpMetric : std::false_type {};

template <class Value>
struct HasDumpMetric<
    Value, utils::void_t<decltype(DumpMetric(std::declval<Value&>()))>>
    : std::true_type {};

template <class Value, class = utils::void_t<>>
struct HasResetMetric : std::false_type {};

template <class Value>
struct HasResetMetric<
    Value, utils::void_t<decltype(ResetMetric(std::declval<Value&>()))>>
    : std::true_type {};

template <typename Metric>
typename std::enable_if<std::is_integral<Metric>::value,
                        formats::json::ValueBuilder>::type
DumpMetric(const Metric&) {
  static_assert(!sizeof(Metric),
                "Type is not atomic, use std::atomic<T> instead");
}

struct MetricInfo final {
  /* stores std::shared_ptr<T> as user type might be non-copyable (e.g.
   * std::atomic). std::any requires copy constructor.
   */
  std::any data_;

  std::string path;
  std::function<formats::json::ValueBuilder(std::any&)> dump_func;
  std::function<void(std::any&)> reset_func;
};

void RegisterMetricInfo(std::type_index ti, MetricInfo&& metric_info);

template <typename Metric>
formats::json::ValueBuilder DumpAnyMetric(std::any& data) {
  static_assert(HasDumpMetric<Metric>::value,
                "There is no `DumpMetric(Metric& / const Metric&)` "
                "in namespace of `Metric`.  "
                "You have not provided a `DumpMetric` function overload.");
  return DumpMetric(*std::any_cast<std::shared_ptr<Metric>&>(data));
}

template <typename Metric>
void ResetAnyMetric(std::any& data) {
  if constexpr (HasResetMetric<Metric>::value) {
    ResetMetric(*std::any_cast<std::shared_ptr<Metric>&>(data));
  }
}

template <typename Metric>
void RegisterTag(const MetricTag<Metric>& tag) {
  RegisterMetricInfo(typeid(Metric), MetricInfo{
                                         std::make_shared<Metric>(),
                                         tag.GetPath(),
                                         DumpAnyMetric<Metric>,
                                         ResetAnyMetric<Metric>,
                                     });
}

}  // namespace impl

template <typename Metric>
MetricTag<Metric>::MetricTag(const std::string& path) : path_(path) {
  impl::RegisterTag(*this);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
