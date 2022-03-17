#pragma once

#include <atomic>
#include <functional>
#include <initializer_list>
#include <list>
#include <string>
#include <vector>

#include <userver/engine/shared_mutex.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/clang_format_workarounds.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

struct StatisticsRequest {
  std::string prefix;
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

class Storage final {
 public:
  Storage();

  Storage(const Storage&) = delete;

  // Creates new Json::Value and calls every registered extender func over it.
  formats::json::ValueBuilder GetAsJson(const StatisticsRequest& request) const;

  // Must be called from StatisticsStorage only. Don't call it from user
  // components.
  void StopRegisteringExtenders();

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
