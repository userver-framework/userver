#include <utils/statistics/storage.hpp>

#include <shared_mutex>
#include <string>
#include <utility>

#include <boost/algorithm/string/predicate.hpp>

#include <logging/log.hpp>
#include <utils/assert.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

namespace {

const std::string kVersionField = "$version";
constexpr int kVersion = 2;

}  // namespace

namespace utils::statistics {

Entry::Entry(Entry&& other) noexcept
    : storage_(other.storage_), iterator_(other.iterator_) {
  other.storage_ = nullptr;
}

Entry::~Entry() {
  if (storage_) {
    LOG_DEBUG() << "Unregistering automatically";
  }
  Unregister();
}

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

Storage::Storage() : may_register_extenders_(true) {}

formats::json::ValueBuilder Storage::GetAsJson(
    const StatisticsRequest& request) const {
  formats::json::ValueBuilder result;
  result[kVersionField] = kVersion;

  std::shared_lock<std::shared_timed_mutex> lock(mutex_);

  for (const auto& it : extender_funcs_) {
    const auto& func_prefix = it.first;
    if (boost::algorithm::starts_with(func_prefix, request.prefix) ||
        (!func_prefix.empty() &&
         boost::algorithm::starts_with(request.prefix, func_prefix))) {
      LOG_DEBUG() << "Getting statistics for prefix=" << func_prefix;
      SetSubField(result, func_prefix, it.second(request));
    }
  }

  return result;
}

void Storage::StopRegisteringExtenders() { may_register_extenders_ = false; }

Entry Storage::RegisterExtender(std::string prefix, ExtenderFunc func) {
  UASSERT_MSG(may_register_extenders_.load(),
              "You may not register statistics extender outside of component "
              "constructors");

  std::lock_guard<std::shared_timed_mutex> lock(mutex_);
  auto res = extender_funcs_.emplace(extender_funcs_.end(), std::move(prefix),
                                     std::move(func));
  return Entry(*this, res);
}

void Storage::UnregisterExtender(StorageIterator iterator) noexcept {
  std::lock_guard<std::shared_timed_mutex> lock(mutex_);
  extender_funcs_.erase(iterator);
}

}  // namespace utils::statistics
