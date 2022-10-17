#pragma once

/// @file userver/utils/statistics/storage.hpp
/// @brief @copybrief utils::statistics::Storage

#include <atomic>
#include <functional>
#include <initializer_list>
#include <list>
#include <string>
#include <variant>
#include <vector>

#include <userver/engine/shared_mutex.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Label {
 public:
  Label() = default;
  Label(std::string name, std::string value);

  explicit operator bool() { return !name_.empty(); }

  const std::string& Name() const { return name_; }
  const std::string& Value() const& { return value_; }
  std::string& Value() & { return value_; }
  std::string&& Value() && { return std::move(value_); }

 private:
  std::string name_;
  std::string value_;
};

bool operator<(const Label& x, const Label& y) noexcept;
bool operator==(const Label& x, const Label& y) noexcept;

struct StatisticsRequest final {
  std::string prefix;
  std::vector<Label> labels{};
  std::string path{};
};

using ExtenderFunc =
    std::function<formats::json::ValueBuilder(const StatisticsRequest&)>;

namespace impl {

struct MetricsSource final {
  std::string prefix_path;
  std::vector<std::string> path_segments;
  ExtenderFunc extender;
};

using StorageData = std::list<MetricsSource>;
using StorageIterator = StorageData::iterator;

}  // namespace impl

class BaseExposeFormatBuilder {
 public:
  using MetricValue = std::variant<std::int64_t, double>;

  virtual ~BaseExposeFormatBuilder() = default;

  virtual void HandleMetric(std::string_view path,
                            const std::vector<Label>& labels,
                            const MetricValue& value) = 0;
};

/// @ingroup userver_clients
///
/// Storage of metrics, usually retrieved from components::StatisticsStorage.
class Storage final {
 public:
  Storage();

  Storage(const Storage&) = delete;

  /// Creates new Json::Value and calls every registered extender func over it.
  formats::json::ValueBuilder GetAsJson(const StatisticsRequest& request) const;

  /// Visits all the metrics and calls `out.HandleMetric` for each metric.
  void VisitMetrics(BaseExposeFormatBuilder& out,
                    const StatisticsRequest& request = {}) const;

  /// @cond
  /// Must be called from StatisticsStorage only. Don't call it from user
  /// components.
  void StopRegisteringExtenders();
  /// @endcond

  Entry RegisterExtender(std::string prefix, ExtenderFunc func);

  Entry RegisterExtender(std::vector<std::string> prefix, ExtenderFunc func);

  Entry RegisterExtender(std::initializer_list<std::string> prefix,
                         ExtenderFunc func);

  void UnregisterExtender(impl::StorageIterator iterator) noexcept;

 private:
  Entry DoRegisterExtender(impl::MetricsSource&& source);

  std::atomic<bool> may_register_extenders_;
  impl::StorageData metrics_sources_;
  mutable engine::SharedMutex mutex_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
