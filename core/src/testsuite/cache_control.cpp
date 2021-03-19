#include <testsuite/cache_control.hpp>

#include <tracing/span.hpp>
#include <utils/algo.hpp>

namespace {

constexpr std::string_view kInvalidatorSpanTag = "cache_invalidate";

}  // namespace

namespace testsuite {

CacheControl::CacheControl(PeriodicUpdatesMode mode)
    : periodic_updates_mode_(mode) {}

void CacheControl::RegisterCache(cache::CacheUpdateTrait& cache) {
  std::lock_guard lock(mutex_);
  caches_.emplace_back(cache);
}

void CacheControl::UnregisterCache(cache::CacheUpdateTrait& cache) {
  std::lock_guard lock(mutex_);
  utils::EraseIf(caches_, [&](auto ref) { return &ref.get() == &cache; });
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

  for (const auto& cache : caches_) {
    if (names_blocklist.count(cache.get().Name()) > 0) continue;
    tracing::Span span(std::string{kInvalidatorSpanTag});
    cache.get().Update(update_type);
  }
}

void CacheControl::InvalidateCaches(
    cache::UpdateType update_type,
    const std::unordered_set<std::string>& names) {
  if (names.empty()) return;
  std::lock_guard lock(mutex_);

  for (const auto& cache : caches_) {
    if (names.count(cache.get().Name()) > 0) {
      tracing::Span span(std::string{kInvalidatorSpanTag});
      cache.get().Update(update_type);
      cache.get().WriteDumpSyncDebug();  // TODO TAXICOMMON-3476 remove
    }
  }
}

void CacheControl::WriteCacheDumps(
    const std::unordered_set<std::string>& cache_names) {
  std::lock_guard lock(mutex_);

  for (const auto& cache : caches_) {
    if (cache_names.count(cache.get().Name()) > 0) {
      cache.get().WriteDumpSyncDebug();
    }
  }
}

void CacheControl::ReadCacheDumps(
    const std::unordered_set<std::string>& cache_names) {
  std::lock_guard lock(mutex_);

  for (const auto& cache : caches_) {
    if (cache_names.count(cache.get().Name()) > 0) {
      cache.get().ReadDumpSyncDebug();
    }
  }
}

CacheInvalidatorHolder::CacheInvalidatorHolder(CacheControl& cache_control,
                                               cache::CacheUpdateTrait& cache)
    : cache_control_(cache_control), cache_(cache) {
  cache_control_.RegisterCache(cache_);
}

CacheInvalidatorHolder::~CacheInvalidatorHolder() {
  cache_control_.UnregisterCache(cache_);
}

}  // namespace testsuite
