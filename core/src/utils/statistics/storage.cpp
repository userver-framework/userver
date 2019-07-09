#include <utils/statistics/storage.hpp>

#include <shared_mutex>
#include <string>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <logging/log.hpp>
#include <utils/assert.hpp>

namespace {

const std::string kVersionField = "$version";
constexpr int kVersion = 2;

void UpdateFields(formats::json::ValueBuilder& object,
                  formats::json::ValueBuilder value) {
  for (auto it = value.begin(); it != value.end(); ++it) {
    const auto& name = it.GetName();
    object[name] = value[name];
  }
}

void SetSubField(formats::json::ValueBuilder& object,
                 std::list<std::string> fields,
                 formats::json::ValueBuilder value) {
  if (fields.empty()) {
    UpdateFields(object, std::move(value));
  } else {
    const auto field = std::move(fields.front());
    fields.pop_front();
    auto subobj = object[field];
    SetSubField(subobj, std::move(fields), std::move(value));
  }
}

void SetSubField(formats::json::ValueBuilder& object, const std::string& path,
                 formats::json::ValueBuilder value) {
  std::list<std::string> fields;
  boost::split(fields, path, [](char c) { return c == '.'; });
  SetSubField(object, std::move(fields), std::move(value));
}

}  // namespace

namespace utils {
namespace statistics {

Entry::Entry(Entry&& other) noexcept
    : storage_(other.storage_), iterator_(other.iterator_) {
  other.storage_ = nullptr;
}

Entry::~Entry() { UASSERT(storage_ == nullptr); }

void Entry::Unregister() noexcept {
  if (storage_) {
    storage_->UnregisterExtender(iterator_);
    storage_ = nullptr;
  }
}

Entry& Entry::operator=(Entry&& other) noexcept {
  Unregister();
  std::swap(storage_, other.storage_);
  std::swap(iterator_, other.iterator_);
  return *this;
}

formats::json::ValueBuilder Storage::GetAsJson(
    const std::string& prefix, const StatisticsRequest& request) const {
  formats::json::ValueBuilder result;
  result[kVersionField] = kVersion;

  std::shared_lock<std::shared_timed_mutex> lock(mutex_);

  for (const auto& it : extender_funcs_) {
    const auto& func_prefix = it.first;
    if (boost::algorithm::starts_with(func_prefix, prefix) ||
        boost::algorithm::starts_with(prefix, func_prefix)) {
      LOG_DEBUG() << "Getting statistics for prefix=" << func_prefix;
      SetSubField(result, func_prefix, it.second(request));
    }
  }

  return result;
}

Entry Storage::RegisterExtender(std::string prefix, ExtenderFunc func) {
  std::lock_guard<std::shared_timed_mutex> lock(mutex_);
  auto res = extender_funcs_.emplace(extender_funcs_.end(), std::move(prefix),
                                     std::move(func));
  return Entry(*this, res);
}

void Storage::UnregisterExtender(StorageIterator iterator) noexcept {
  std::lock_guard<std::shared_timed_mutex> lock(mutex_);
  extender_funcs_.erase(iterator);
}

}  // namespace statistics
}  // namespace utils
