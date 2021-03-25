#include <testsuite/cache_control.hpp>

#include <tracing/span.hpp>
#include <utils/algo.hpp>

#include <fmt/format.h>

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

void CacheControl::InvalidateCaches(cache::UpdateType update_type,
                                    std::unordered_set<std::string> names) {
  ForEachCache(std::move(names), [&](cache::CacheUpdateTrait& cache) {
    tracing::Span span(std::string{kInvalidatorSpanTag});
    cache.Update(update_type);
  });
}

void CacheControl::WriteCacheDumps(
    std::unordered_set<std::string> cache_names) {
  ForEachCache(std::move(cache_names), [](cache::CacheUpdateTrait& cache) {
    cache.WriteDumpSyncDebug();
  });
}

void CacheControl::ReadCacheDumps(std::unordered_set<std::string> cache_names) {
  ForEachCache(std::move(cache_names), [](cache::CacheUpdateTrait& cache) {
    cache.ReadDumpSyncDebug();
  });
}

void CacheControl::ForEachCache(
    std::unordered_set<std::string> cache_names,
    std::function<void(cache::CacheUpdateTrait&)> callback) {
  std::lock_guard lock(mutex_);

  // It's important that we walk the caches in the order of their registration,
  // that is, in the order of `caches_`. Otherwise if we update a "later" cache
  // before an "earlier" cache, the "later" one might read old data from
  // the "earlier" one and will not be fully updated.
  for (const auto& cache : caches_) {
    const auto it = cache_names.find(cache.get().Name());
    if (it != cache_names.end()) {
      callback(cache.get());
      cache_names.erase(it);
    }
  }

  YTX_INVARIANT(cache_names.empty(),
                fmt::format("Some of the requested caches do not exist: {}",
                            fmt::join(cache_names, ", ")));
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
