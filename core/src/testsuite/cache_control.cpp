#include <userver/testsuite/cache_control.hpp>

#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_update_trait.hpp>
#include <userver/components/component_context.hpp>
#include <userver/formats/json.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fast_scope_guard.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

CacheControl::CacheControl(impl::PeriodicUpdatesMode mode)
    : periodic_updates_mode_(mode) {}

bool CacheControl::IsPeriodicUpdateEnabled(
    const cache::Config& cache_config, const std::string& cache_name) const {
  const auto is_periodic_update_forced = cache_config.force_periodic_update;

  bool enabled = true;
  std::string reason;
  if (is_periodic_update_forced) {
    enabled = *is_periodic_update_forced;
    reason = "config";
  } else if (periodic_updates_mode_ == impl::PeriodicUpdatesMode::kDefault) {
    enabled = true;
    reason = "default";
  } else {
    enabled = (periodic_updates_mode_ == impl::PeriodicUpdatesMode::kEnabled);
    reason = "global config";
  }

  const auto* state = enabled ? "enabled" : "disabled";
  LOG_DEBUG() << cache_name << " periodic update is " << state << " by "
              << reason;
  return enabled;
}

void CacheControl::DoResetCache(const CacheInfo& info,
                                cache::UpdateType update_type) {
  TESTPOINT(std::string("reset-cache-") + info.name,
            formats::json::MakeObject("update_type", ToString(update_type)));

  std::optional<tracing::Span> span;
  if (info.needs_span) {
    span.emplace("reset/" + info.name);
  }
  info.reset(update_type);
}

void CacheControl::ResetAllCaches(
    cache::UpdateType update_type,
    const std::unordered_set<std::string>& force_incremental_names) {
  const auto sp =
      tracing::ScopeTime::CreateOptionalScopeTime("reset_all_caches");
  auto caches = caches_.Lock();

  for (const auto& cache : *caches) {
    const auto cache_update_type = force_incremental_names.count(cache.name)
                                       ? cache::UpdateType::kIncremental
                                       : update_type;
    DoResetCache(cache, cache_update_type);
  }
}

void CacheControl::ResetCaches(
    cache::UpdateType update_type,
    std::unordered_set<std::string> reset_only_names,
    const std::unordered_set<std::string>& force_incremental_names) {
  const auto sp = tracing::ScopeTime::CreateOptionalScopeTime("reset_caches");
  auto caches = caches_.Lock();

  // It's important that we walk the caches in the order of their registration,
  // that is, in the order of `caches_`. Otherwise, if we update a "later" cache
  // before an "earlier" cache, the "later" one might read old data from
  // the "earlier" one and will not be fully updated.
  for (const auto& cache : *caches) {
    const auto it = reset_only_names.find(cache.name);
    if (it != reset_only_names.end()) {
      const auto cache_update_type = force_incremental_names.count(cache.name)
                                         ? cache::UpdateType::kIncremental
                                         : update_type;
      DoResetCache(cache, cache_update_type);
      reset_only_names.erase(it);
    }
  }

  UINVARIANT(reset_only_names.empty(),
             fmt::format("Some of the requested caches do not exist: {}",
                         fmt::join(reset_only_names, ", ")));
}

CacheResetRegistration CacheControl::RegisterPeriodicCache(
    cache::CacheUpdateTrait& cache) {
  auto iter = DoRegisterCache(CacheInfo{
      /*name=*/std::string{cache.Name()},
      /*reset=*/
      [&cache](cache::UpdateType update_type) {
        cache.UpdateSyncDebug(update_type);
      },
      /*needs_span=*/false,
  });
  return CacheResetRegistration(*this, std::move(iter));
}

auto CacheControl::DoRegisterCache(CacheInfo&& info) -> CacheInfoIterator {
  auto caches = caches_.Lock();
  caches->push_back(std::move(info));
  return std::prev(caches->end());
}

void CacheControl::UnregisterCache(CacheInfoIterator iterator) noexcept {
  auto caches = caches_.Lock();
  caches->erase(iterator);
}

CacheResetRegistration::CacheResetRegistration() noexcept = default;

CacheResetRegistration::CacheResetRegistration(
    CacheControl& cache_control,
    CacheControl::CacheInfoIterator cache_info_iterator)
    : cache_control_(&cache_control),
      cache_info_iterator_(std::move(cache_info_iterator)) {}

CacheResetRegistration::CacheResetRegistration(
    CacheResetRegistration&& other) noexcept
    : cache_control_(std::exchange(other.cache_control_, nullptr)),
      cache_info_iterator_(std::move(other.cache_info_iterator_)) {}

CacheResetRegistration& CacheResetRegistration::operator=(
    CacheResetRegistration&& other) noexcept {
  if (&other == this) return *this;
  Unregister();
  cache_control_ = std::exchange(other.cache_control_, nullptr);
  cache_info_iterator_ = std::move(other.cache_info_iterator_);
  return *this;
}

CacheResetRegistration::~CacheResetRegistration() { Unregister(); }

void CacheResetRegistration::Unregister() noexcept {
  if (cache_control_) {
    cache_control_->UnregisterCache(std::move(cache_info_iterator_));
    cache_control_ = nullptr;
  }
}

CacheControl& FindCacheControl(const components::ComponentContext& context) {
  return context.FindComponent<components::TestsuiteSupport>()
      .GetCacheControl();
}

}  // namespace testsuite

USERVER_NAMESPACE_END
