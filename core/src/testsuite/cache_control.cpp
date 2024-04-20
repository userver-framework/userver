#include <userver/testsuite/cache_control.hpp>

#include <optional>

#include <fmt/format.h>
#include <boost/intrusive/list.hpp>

#include <userver/cache/cache_config.hpp>
#include <userver/cache/cache_update_trait.hpp>
#include <userver/components/component_context.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/formats/json.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/intrusive_link_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

constexpr std::string_view kKnownReverseDependency =
    components::DynamicConfigClientUpdater::kName;

struct CacheControl::CacheInfoNode final {
  CacheInfoNode() = default;
  CacheInfoNode(CacheInfoNode&&) = delete;
  CacheInfoNode(const CacheInfoNode&) = delete;
  CacheInfoNode& operator=(CacheInfoNode&&) = delete;
  CacheInfoNode& operator=(const CacheInfoNode&) = delete;

  CacheInfo info;

  std::unordered_set<std::string_view> dependencies{};

  boost::intrusive::list_member_hook<utils::impl::IntrusiveLinkMode> hook{};
};

class CacheControl::CacheResetJob final {
 public:
  CacheResetJob() = delete;
  explicit CacheResetJob(CacheInfoNode& node) noexcept : node{node} {}

  const std::string& Name() const noexcept { return node.info.name; }

  bool HasDependencyOn(const CacheResetJob& other, components::State state) {
    if (node.dependencies.empty()) {
      node.dependencies = state.GetAllDependencies(Name());
    }
    return node.dependencies.count(other.Name());
  }

  void WaitIfDependsOn(const CacheResetJob& other, components::State state) {
    if (&node == &other.node) {
      return;  // Cache can not depend on itself
    }

    if (HasDependencyOn(other, state) && !other.task.IsFinished()) {
      LOG_DEBUG() << Name() << " cache update waits for " << other.Name();
      other.task.Wait();
    }
  }

  CacheInfoNode& node;
  engine::SharedTaskWithResult<void> task{};
};

struct CacheControl::Impl final {
  Impl(impl::PeriodicUpdatesMode mode, ExecPolicy policy,
       std::optional<components::State> components_state)
      : periodic_updates_mode(mode),
        execution_policy(policy),
        state(components_state) {
    UASSERT(execution_policy == ExecPolicy::kSequential || components_state);
  }

  using MemberHook = boost::intrusive::member_hook<
      CacheControl::CacheInfoNode,
      boost::intrusive::list_member_hook<utils::impl::IntrusiveLinkMode>,
      &CacheControl::CacheInfoNode::hook>;

  // We need the order of cache registrations to minimize work in DoResetCaches
  // and to have a stable CacheInfoIterator.
  using List =
      boost::intrusive::list<CacheInfoNode, MemberHook,
                             boost::intrusive::constant_time_size<true>>;

  const impl::PeriodicUpdatesMode periodic_updates_mode;
  const ExecPolicy execution_policy;
  std::optional<components::State> state;
  concurrent::Variable<List> caches{};
};

CacheControl::CacheControl(impl::PeriodicUpdatesMode mode, UnitTests)
    : impl_(std::make_unique<Impl>(mode, ExecPolicy::kSequential,
                                   std::nullopt)) {}

CacheControl::CacheControl(impl::PeriodicUpdatesMode mode,
                           ExecPolicy execution_policy, components::State state)
    : impl_(std::make_unique<Impl>(mode, execution_policy, state)) {}

CacheControl::~CacheControl() = default;

bool CacheControl::IsPeriodicUpdateEnabled(
    const cache::Config& cache_config, const std::string& cache_name) const {
  const auto is_periodic_update_forced = cache_config.force_periodic_update;

  bool enabled = true;
  std::string reason;
  if (is_periodic_update_forced) {
    enabled = *is_periodic_update_forced;
    reason = "config";
  } else if (impl_->periodic_updates_mode ==
             impl::PeriodicUpdatesMode::kDefault) {
    enabled = true;
    reason = "default";
  } else {
    enabled =
        (impl_->periodic_updates_mode == impl::PeriodicUpdatesMode::kEnabled);
    reason = "global config";
  }

  LOG_DEBUG() << cache_name << " periodic update is "
              << (enabled ? "enabled" : "disabled") << " by " << reason;
  return enabled;
}

void CacheControl::DoResetSingleCache(
    const CacheInfo& info, cache::UpdateType update_type,
    const std::unordered_set<std::string>& force_incremental_names) {
  update_type = force_incremental_names.count(info.name)
                    ? cache::UpdateType::kIncremental
                    : update_type;

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
    const std::unordered_set<std::string>& force_incremental_names,
    const std::unordered_set<std::string>& exclude_names) {
  const auto sp =
      tracing::ScopeTime::CreateOptionalScopeTime("reset_all_caches");

  DoResetCaches(update_type, nullptr, force_incremental_names, &exclude_names);
}

void CacheControl::ResetCaches(
    cache::UpdateType update_type,
    std::unordered_set<std::string> reset_only_names,
    const std::unordered_set<std::string>& force_incremental_names) {
  const auto sp = tracing::ScopeTime::CreateOptionalScopeTime("reset_caches");

  DoResetCaches(update_type, &reset_only_names, force_incremental_names,
                nullptr);
}

CacheResetRegistration CacheControl::RegisterPeriodicCache(
    cache::CacheUpdateTrait& cache) {
  CacheInfo info;
  info.name = std::string{cache.Name()};
  info.reset = [&cache](cache::UpdateType update_type) {
    cache.UpdateSyncDebug(update_type);
  };
  info.needs_span = false;

  auto iter = DoRegisterCache(std::move(info));
  return CacheResetRegistration(*this, std::move(iter));
}

void CacheControl::DoResetCaches(
    cache::UpdateType update_type,
    std::unordered_set<std::string>* reset_only_names,
    const std::unordered_set<std::string>& force_incremental_names,
    const std::unordered_set<std::string>* exclude_names) {
  // It's important to update a cache X that depends on cache Y after updating
  // the cache Y. Otherwise X may read old data from Y and will not be
  // properly updated.

  // No concurrent updates for unit tests as we have no access to
  // component system in mocked environments.
  UASSERT(!(exclude_names && reset_only_names));
  switch (impl_->execution_policy) {
    case ExecPolicy::kSequential: {
      auto caches = impl_->caches.Lock();
      for (auto& node : *caches) {
        if (exclude_names && exclude_names->count(node.info.name)) {
          continue;
        }
        if (reset_only_names && !reset_only_names->erase(node.info.name)) {
          continue;
        }

        DoResetSingleCache(node.info, update_type, force_incremental_names);
      }
    } break;
    case ExecPolicy::kConcurrent:
      DoResetCachesConcurrently(update_type, reset_only_names,
                                force_incremental_names, exclude_names);
      break;
  }

  UINVARIANT(!reset_only_names || reset_only_names->empty(),
             fmt::format("Some of the requested caches do not exist: {}",
                         fmt::join(*reset_only_names, ", ")));
}

void CacheControl::DoResetCachesConcurrently(
    cache::UpdateType update_type,
    std::unordered_set<std::string>* reset_only_names,
    const std::unordered_set<std::string>& force_incremental_names,
    const std::unordered_set<std::string>* exclude_names) {
  UASSERT(impl_->state);
  const auto& state = *impl_->state;

  auto caches = impl_->caches.Lock();  // Protects caches[*].dependencies

  engine::SharedMutex tasks_init_mutex;  // should go before async_jobs
  std::vector<CacheResetJob> async_jobs;
  async_jobs.reserve(reset_only_names ? reset_only_names->size()
                                      : caches->size());
  UASSERT(!(exclude_names && reset_only_names));
  for (auto& node : *caches) {
    if (exclude_names && exclude_names->count(node.info.name)) {
      continue;
    }
    if (reset_only_names && !reset_only_names->erase(node.info.name)) {
      continue;
    }

    if (node.info.name == kKnownReverseDependency) {
      DoResetSingleCache(node.info, update_type, force_incremental_names);
    } else {
      async_jobs.emplace_back(node);
    }
  }

  {
    const std::unique_lock lock{tasks_init_mutex};
    for (std::size_t i = 0; i < async_jobs.size(); ++i) {
      async_jobs[i].task = engine::SharedAsyncNoSpan([i, &async_jobs,
                                                      &tasks_init_mutex,
                                                      &force_incremental_names,
                                                      update_type, state] {
        const std::shared_lock lock{tasks_init_mutex};

        auto& job = async_jobs[i];
        for (auto& other_job : async_jobs) {
          job.WaitIfDependsOn(other_job, state);
        }

        DoResetSingleCache(job.node.info, update_type, force_incremental_names);
      });
    }
  }

  LOG_INFO() << "Waiting for cache updates";
  for (auto& job : async_jobs) {
    // Wait() before Get() to avoid an exception destroying elements of the
    // `async_jobs` with active tasks referencing `async_jobs` elements.
    job.task.Wait();
  }
  for (auto& job : async_jobs) {
    job.task.Get();
  }
}

auto CacheControl::DoRegisterCache(CacheInfo&& info) -> CacheInfoIterator {
  // TODO: same component could be added more than once
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
