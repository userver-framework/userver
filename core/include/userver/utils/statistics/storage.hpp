#pragma once

/// @file userver/utils/statistics/storage.hpp
/// @brief @copybrief utils::statistics::Storage

#include <atomic>
#include <functional>
#include <initializer_list>
#include <list>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <userver/engine/shared_mutex.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/metric_value.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Used in legacy statistics extenders
struct StatisticsRequest final {};

/// @brief Class describing the request for metrics data.
///
/// Metric path and metric name are the same thing. For example for code like
///
/// @code
/// writer["a"]["b"]["c"] = 42;
/// @endcode
///
/// the metric name is "a.b.c" and in Prometheus format it would be escaped like
/// "a_b_c{} 42".
///
/// @note `add_labels` should not match labels from Storage, otherwise the
/// returned metrics may not be read by metrics server.
class Request final {
 public:
  using AddLabels = std::unordered_map<std::string, std::string>;

  /// Default request without parameters. Equivalent to requesting all the
  /// metrics without adding or requiring any labels.
  Request() = default;

  /// Makes request for metrics whose path starts with the `prefix`.
  static Request MakeWithPrefix(const std::string& prefix,
                                AddLabels add_labels = {},
                                std::vector<Label> require_labels = {});

  /// Makes request for metrics whose path is `path`.
  static Request MakeWithPath(const std::string& path,
                              AddLabels add_labels = {},
                              std::vector<Label> require_labels = {});

  /// Return metrics whose path matches with this `prefix`
  const std::string prefix{};

  /// Enum for different match types of the `prefix`
  enum class PrefixMatch {
    kNoop,        ///< Do not match, the `prefix` is empty
    kExact,       ///< `prefix` equal to path
    kStartsWith,  ///< Metric path starts with `prefix`
  };

  /// Match type of the `prefix`
  const PrefixMatch prefix_match_type = PrefixMatch::kNoop;

  /// Require those labels in the metric
  const std::vector<Label> require_labels{};

  /// Add those labels to each returned metric
  const AddLabels add_labels{};

 private:
  Request(std::string prefix_in, PrefixMatch path_match_type_in,
          std::vector<Label> require_labels_in, AddLabels add_labels_in);
};

using ExtenderFunc =
    std::function<formats::json::ValueBuilder(const StatisticsRequest&)>;

using WriterFunc = std::function<void(Writer&)>;

namespace impl {

struct MetricsSource final {
  std::string prefix_path;
  std::vector<std::string> path_segments;
  ExtenderFunc extender;

  WriterFunc writer;
  std::vector<Label> writer_labels;
};

using StorageData = std::list<MetricsSource>;
using StorageIterator = StorageData::iterator;

inline constexpr bool kCheckSubscriptionUB = utils::impl::kEnableAssert;

}  // namespace impl

class BaseFormatBuilder {
 public:
  virtual ~BaseFormatBuilder();

  virtual void HandleMetric(std::string_view path, LabelsSpan labels,
                            const MetricValue& value) = 0;
};

/// @ingroup userver_clients
///
/// Storage of metrics, usually retrieved from components::StatisticsStorage.
///
/// See utils::statistics::Writer for an information on how to write metrics.
class Storage final {
 public:
  Storage();

  Storage(const Storage&) = delete;

  /// Creates new Json::Value and calls every deprecated registered extender
  /// func over it.
  ///
  /// @deprecated Use VisitMetrics instead.
  formats::json::Value GetAsJson() const;

  /// Visits all the metrics and calls `out.HandleMetric` for each metric.
  void VisitMetrics(BaseFormatBuilder& out, const Request& request = {}) const;

  /// @cond
  /// Must be called from StatisticsStorage only. Don't call it from user
  /// components.
  void StopRegisteringExtenders();
  /// @endcond

  /// @brief Add a writer function. Note that `func` is called concurrently with
  /// other code, so it should be tharead\coroutine safe.
  Entry RegisterWriter(std::string common_prefix, WriterFunc func,
                       std::vector<Label> add_labels = {});

  /// @deprecated Use RegisterWriter instead.
  Entry RegisterExtender(std::string prefix, ExtenderFunc func);

  void UnregisterExtender(impl::StorageIterator iterator,
                          impl::UnregisteringKind kind) noexcept;

 private:
  Entry DoRegisterExtender(impl::MetricsSource&& source);

  std::atomic<bool> may_register_extenders_;
  impl::StorageData metrics_sources_;
  mutable engine::SharedMutex mutex_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
