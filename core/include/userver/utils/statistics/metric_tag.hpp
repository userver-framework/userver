#pragma once

#include <string>
#include <typeinfo>
#include <utility>

#include <userver/utils/statistics/metric_tag_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Metric description
///
/// Use `MetricTag<Metric>` for declarative style of metric registration and
/// call `MetricStorage::GetMetric` for accessing metric data. Please
/// note that metrics can be accessed from multiple coroutines, so `Metric` must
/// be thread-safe (e.g. std::atomic<T>, rcu::Variable<T>, rcu::RcuMap<T>,
/// concurrent::Variable<T>, etc.).
///
/// A custom metric type must be default-constructible and have the following
/// free function defined:
/// @code
/// void DumpMetric(utils::statistics::Writer&, const Metric&)
/// @endcode
template <typename Metric>
class MetricTag final {
 public:
  /// Register metric, passing a copy of `args` to the constructor of `Metric`
  template <typename... Args>
  explicit MetricTag(const std::string& path, Args&&... args)
      : key_{typeid(Metric), path} {
    impl::RegisterMetricInfo(
        key_, impl::MakeMetricFactory<Metric>(std::forward<Args>(args)...));
  }

  std::string GetPath() const { return key_.path; }

 private:
  friend class MetricsStorage;

  const impl::MetricKey key_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
