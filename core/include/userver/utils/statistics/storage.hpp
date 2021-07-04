#pragma once

#include <atomic>
#include <functional>
#include <list>
#include <string>
#include <utility>

#include <shared_mutex>
#include <userver/formats/json/value_builder.hpp>

namespace utils {
namespace statistics {

struct StatisticsRequest {
  std::string prefix;
};
using ExtenderFunc =
    std::function<formats::json::ValueBuilder(const StatisticsRequest&)>;

using StorageData = std::list<std::pair<std::string, ExtenderFunc>>;
using StorageIterator = StorageData::iterator;

class Storage;

class Entry final {
 public:
  Entry() = default;
  Entry(const Entry& other) = delete;
  Entry(Entry&& other) noexcept;

  ~Entry();

  Entry& operator=(Entry&& other) noexcept;

  void Unregister() noexcept;

 private:
  explicit Entry(Storage& storage, StorageIterator iterator)
      : storage_(&storage), iterator_(iterator) {}

 private:
  Storage* storage_{nullptr};
  StorageIterator iterator_;

  friend class Storage;  // in RegisterExtender()
};

class Storage final {
 public:
  Storage();

  Storage(const Storage&) = delete;

  // Creates new Json::Value and calls every registered extender func over it.
  formats::json::ValueBuilder GetAsJson(const StatisticsRequest& request) const;

  // Must be called from StatisticsStorage only. Don't call it from user
  // components.
  void StopRegisteringExtenders();

  [[nodiscard]] Entry RegisterExtender(std::string prefix, ExtenderFunc func);

  void UnregisterExtender(StorageIterator iterator) noexcept;

 private:
  std::atomic<bool> may_register_extenders_;
  StorageData extender_funcs_;
  mutable std::shared_timed_mutex mutex_;
};

}  // namespace statistics
}  // namespace utils
