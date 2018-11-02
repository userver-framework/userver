#pragma once

#include <functional>
#include <list>
#include <string>
#include <utility>

#include <formats/json/value_builder.hpp>
#include <shared_mutex>

namespace utils {
namespace statistics {

struct StatisticsRequest {};
using ExtenderFunc =
    std::function<formats::json::ValueBuilder(const StatisticsRequest&)>;

using StorageData = std::list<std::pair<std::string, ExtenderFunc>>;
using StorageIterator = StorageData::iterator;

class Storage;

class Entry {
 public:
  Entry() : storage_(nullptr) {}
  Entry(const Entry& other) = delete;
  Entry(Entry&& other) noexcept;

  ~Entry();

  Entry& operator=(Entry&& other);

  void Unregister();

 private:
  explicit Entry(Storage& storage, StorageIterator iterator)
      : storage_(&storage), iterator_(iterator) {}

 private:
  Storage* storage_;
  StorageIterator iterator_;

  friend class Storage;  // in RegisterExtender()
};

class Storage {
 public:
  // Creates new Json::Value and calls every registered extender func over it.
  formats::json::ValueBuilder GetAsJson(const std::string& prefix,
                                        const StatisticsRequest& request) const;

  __attribute__((warn_unused_result)) Entry RegisterExtender(std::string prefix,
                                                             ExtenderFunc func);

  void UnregisterExtender(StorageIterator iterator);

 private:
  StorageData extender_funcs_;
  mutable std::shared_timed_mutex mutex_;
};

}  // namespace statistics
}  // namespace utils
