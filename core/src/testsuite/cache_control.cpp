#include <userver/testsuite/cache_control.hpp>

#include <fmt/format.h>
#include <boost/intrusive/list.hpp>

#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_update_trait.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/formats/json.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

struct CacheControl::CacheInfoNode final {
  CacheInfo info;
  boost::intrusive::list_member_hook<utils::impl::IntrusiveLinkMode> hook{};
};

struct CacheControl::Impl final {
  explicit Impl(impl::PeriodicUpdatesMode periodic_updates_mode)
      : periodic_updates_mode_(periodic_updates_mode) {}

  using MemberHook = boost::intrusive::member_hook<
      CacheControl::CacheInfoNode,
      boost::intrusive::list_member_hook<utils::impl::IntrusiveLinkMode>,
      &CacheControl::CacheInfoNode::hook>;

  using List =
      boost::intrusive::list<CacheInfoNode, MemberHook,
                             boost::intrusive::constant_time_size<false>>;

  const impl::PeriodicUpdatesMode periodic_updates_mode_;
  concurrent::Variable<List> caches{};
};

CacheControl::CacheControl(impl::PeriodicUpdatesMode mode)
    : impl_(std::make_unique<Impl>(mode)) {}

CacheControl::~CacheControl() = default;

bool CacheControl::IsPeriodicUpdateEnabled(
    const cache::Config& cache_config, const std::string& cache_name) const {
  const auto is_periodic_update_forced = cache_config.force_periodic_update;

  bool enabled = true;
  std::string reason;
  if (is_periodic_update_forced) {
    enabled = *is_periodic_update_forced;
    reason = "config";
  } else if (impl_->periodic_updates_mode_ ==
             impl::PeriodicUpdatesMode::kDefault) {
    enabled = true;
    reason = "default";
  } else {
    enabled =
        (impl_->periodic_updates_mode_ == impl::PeriodicUpdatesMode::kEnabled);
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
  auto caches = impl_->caches.Lock();

  for (const auto& cache : *caches) {
    const auto cache_update_type =
        force_incremental_names.count(cache.info.name)
            ? cache::UpdateType::kIncremental
            : update_type;
    DoResetCache(cache.info, cache_update_type);
  }
}

void CacheControl::ResetCaches(
    cache::UpdateType update_type,
    std::unordered_set<std::string> reset_only_names,
    const std::unordered_set<std::string>& force_incremental_names) {
  const auto sp = tracing::ScopeTime::CreateOptionalScopeTime("reset_caches");
  auto caches = impl_->caches.Lock();

  // It's important that we walk the caches in the order of their registration,
  // that is, in the order of `impl_->caches`. Otherwise, if we update a "later"
  // cache before an "earlier" cache, the "later" one might read old data from
  // the "earlier" one and will not be fully updated.
  for (const auto& cache : *caches) {
    const auto it = reset_only_names.find(cache.info.name);
    if (it != reset_only_names.end()) {
      const auto cache_update_type =
          force_incremental_names.count(cache.info.name)
              ? cache::UpdateType::kIncremental
              : update_type;
      DoResetCache(cache.info, cache_update_type);
      reset_only_names.erase(it);
    }
  }

  UINVARIANT(reset_only_names.empty(),
             fmt::format("Some of the requested caches do not exist: {}",
                         fmt::join(reset_only_names, ", ")));
}

CacheResetRegistration CacheControl::RegisterPeriodicCache(
    cache::CacheUpdateTrait& cache) {
  CacheInfo info;
  info.component = nullptr;
  info.name = std::string{cache.Name()};
  info.reset = [&cache](cache::UpdateType update_type) {
    cache.UpdateSyncDebug(update_type);
  };
  info.needs_span = false;

  auto iter = DoRegisterCache(std::move(info));
  return CacheResetRegistration(*this, std::move(iter));
}

auto CacheControl::DoRegisterCache(CacheInfo&& info) -> CacheInfoIterator {
  auto node = std::make_unique<CacheInfoNode>();
  node->info = std::move(info);

  auto caches = impl_->caches.Lock();
  caches->push_back(*node);
  return node.release();
}

void CacheControl::UnregisterCache(CacheInfoIterator iterator) noexcept {
  UASSERT(iterator);
  std::unique_ptr<CacheInfoNode> ptr{iterator};

  auto caches = impl_->caches.Lock();
  caches->erase(Impl::List::s_iterator_to(*ptr));
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
