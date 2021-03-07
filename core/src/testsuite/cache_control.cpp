#include <testsuite/cache_control.hpp>

#include <algorithm>

#include <tracing/span.hpp>
#include <utils/assert.hpp>

namespace {

constexpr std::string_view kInvalidatorSpanTag = "cache_invalidate";

}  // namespace

namespace testsuite {

CacheControl::CacheControl(PeriodicUpdatesMode mode)
    : periodic_updates_mode_(mode) {}

void CacheControl::RegisterCacheInvalidator(cache::CacheUpdateTrait& owner,
                                            Callback callback) {
  std::lock_guard lock(mutex_);
  invalidators_.push_back(Invalidator{&owner, std::move(callback)});
}

void CacheControl::UnregisterCacheInvalidator(cache::CacheUpdateTrait& owner) {
  std::lock_guard lock(mutex_);
  invalidators_.erase(
      std::remove_if(invalidators_.begin(), invalidators_.end(),
                     [owner_ptr = &owner](const Invalidator& invalidator) {
                       return invalidator.owner == owner_ptr;
                     }),
      invalidators_.end());
}

bool CacheControl::IsPeriodicUpdateEnabled(
    const cache::CacheConfigStatic& cache_config,
    const std::string& cache_name) const {
  const auto is_periodic_update_forced = cache_config.force_periodic_update;

  bool enabled = true;
  std::string reason;
  if (is_periodic_update_forced) {
    enabled = *is_periodic_update_forced;
    reason = "config";
  } else if (periodic_updates_mode_ == PeriodicUpdatesMode::kDefault) {
    enabled = true;
    reason = "default";
  } else {
    enabled = (periodic_updates_mode_ == PeriodicUpdatesMode::kEnabled);
    reason = "global config";
  }

  auto state = enabled ? "enabled" : "disabled";
  LOG_DEBUG() << cache_name << " periodic update is " << state << " by "
              << reason;
  return enabled;
}

void CacheControl::InvalidateAllCaches(
    cache::UpdateType update_type,
    const std::unordered_set<std::string>& names_blocklist) {
  std::lock_guard lock(mutex_);

  for (const auto& [owner, callback] : invalidators_) {
    UASSERT(owner);
    if (names_blocklist.count(owner->Name()) > 0) continue;
    tracing::Span span(std::string{kInvalidatorSpanTag});
    callback(*owner, update_type);
  }
}

void CacheControl::InvalidateCaches(
    cache::UpdateType update_type,
    const std::unordered_set<std::string>& names) {
  if (names.empty()) return;

  std::lock_guard lock(mutex_);
  for (const auto& [owner, callback] : invalidators_) {
    UASSERT(owner);
    if (names.count(owner->Name()) > 0) {
      tracing::Span span(std::string{kInvalidatorSpanTag});
      callback(*owner, update_type);
    }
  }
}

CacheInvalidatorHolder::CacheInvalidatorHolder(CacheControl& cache_control,
                                               cache::CacheUpdateTrait& cache)
    : cache_control_(cache_control), cache_(cache) {
  cache_control_.RegisterCacheInvalidator(
      cache_,
      [](cache::CacheUpdateTrait& cache, cache::UpdateType update_type) {
        cache.Update(update_type);
        cache.DumpSyncDebug();
      });
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  cache_control_.UnregisterCacheInvalidator(cache_);
}

}  // namespace testsuite
