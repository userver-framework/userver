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
  invalidators_.emplace_back(owner, std::move(callback));
}

void CacheControl::UnregisterCacheInvalidator(cache::CacheUpdateTrait& owner) {
  std::lock_guard lock(mutex_);
  invalidators_.remove_if([owner_ptr = &owner](const Invalidator& cand) {
    return cand.owner == owner_ptr;
  });
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

void CacheControl::InvalidateAllCaches(cache::UpdateType update_type) {
  std::lock_guard lock(mutex_);

  for (const auto& invalidator : invalidators_) {
    tracing::Span span(std::string{kInvalidatorSpanTag});
    UASSERT(invalidator.owner);
    invalidator.callback(*invalidator.owner, update_type);
  }
}

void CacheControl::InvalidateCaches(cache::UpdateType update_type,
                                    const std::vector<std::string>& names) {
  if (names.empty()) return;

  std::lock_guard lock(mutex_);
  for (const auto& invalidator : invalidators_) {
    if (std::find(names.begin(), names.end(), invalidator.owner->Name()) !=
        names.end()) {
      tracing::Span span(std::string{kInvalidatorSpanTag});
      UASSERT(invalidator.owner);
      invalidator.callback(*invalidator.owner, update_type);
    }
  }
}

CacheControl::Invalidator::Invalidator(cache::CacheUpdateTrait& owner_,
                                       Callback callback_)
    : owner(&owner_), callback(std::move(callback_)) {}

CacheInvalidatorHolder::CacheInvalidatorHolder(CacheControl& cache_control,
                                               cache::CacheUpdateTrait& cache)
    : cache_control_(cache_control), cache_(cache) {
  cache_control_.RegisterCacheInvalidator(
      cache_, [](cache::CacheUpdateTrait& cache,
                 cache::UpdateType update_type) { cache.Update(update_type); });
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  cache_control_.UnregisterCacheInvalidator(cache_);
}

}  // namespace testsuite
