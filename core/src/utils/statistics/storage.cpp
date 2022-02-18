#include <userver/utils/statistics/storage.hpp>

#include <utility>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/text.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

USERVER_NAMESPACE_BEGIN

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

  std::shared_lock lock(mutex_);

  for (const auto& entry : metrics_sources_) {
    if (utils::text::StartsWith(entry.prefix_path, request.prefix) ||
        utils::text::StartsWith(request.prefix, entry.prefix_path)) {
      LOG_DEBUG() << "Getting statistics for prefix=" << entry.prefix_path;
      SetSubField(result, std::vector(entry.path_segments),
                  entry.extender(request));
    }
  }

  return result;
}

void Storage::StopRegisteringExtenders() { may_register_extenders_ = false; }

Entry Storage::RegisterExtender(std::string prefix, ExtenderFunc func) {
  auto prefix_split = SplitPath(prefix);
  return DoRegisterExtender(impl::MetricsSource{
      std::move(prefix), std::move(prefix_split), std::move(func)});
}

Entry Storage::RegisterExtender(std::vector<std::string> prefix,
                                ExtenderFunc func) {
  auto prefix_joined = JoinPath(prefix);
  return DoRegisterExtender(impl::MetricsSource{
      std::move(prefix_joined), std::move(prefix), std::move(func)});
}

Entry Storage::RegisterExtender(std::initializer_list<std::string> prefix,
                                ExtenderFunc func) {
  return RegisterExtender(std::vector(prefix), std::move(func));
}

Entry Storage::DoRegisterExtender(impl::MetricsSource&& source) {
  UASSERT_MSG(may_register_extenders_.load(),
              "You may not register statistics extender outside of component "
              "constructors");

  std::lock_guard lock(mutex_);
  const auto res =
      metrics_sources_.insert(metrics_sources_.end(), std::move(source));
  return Entry(*this, res);
}

void Storage::UnregisterExtender(impl::StorageIterator iterator) noexcept {
  std::lock_guard lock(mutex_);
  metrics_sources_.erase(iterator);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
