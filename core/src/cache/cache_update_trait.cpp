#include <userver/cache/cache_update_trait.hpp>

#include <utility>

#include <cache/cache_dependencies.hpp>
#include <cache/cache_update_trait_impl.hpp>
#include <userver/components/scope.hpp>
#include <userver/dump/helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

CacheUpdateTrait::CacheUpdateTrait(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : CacheUpdateTrait(CacheDependencies::Make(config, context))
{
    context.RegisterScope(components::MakeScope([this] {
        StartPeriodicUpdates();
        return utils::FastScopeGuard([this]() noexcept { StopPeriodicUpdates(); });
    }));
}

CacheUpdateTrait::CacheUpdateTrait(CacheDependencies&& dependencies)
    : impl_(std::make_unique<Impl>(std::move(dependencies), *this))
{}

CacheUpdateTrait::~CacheUpdateTrait()
{
    // In case of tests scope is not registered,
    // and updates must be explicitly stopped here
    StopPeriodicUpdates();
}

void CacheUpdateTrait::UpdateSyncDebug(UpdateType update_type) { impl_->UpdateSyncDebug(update_type); }

void CacheUpdateTrait::InvalidateAsync(UpdateType update_type) { impl_->InvalidateAsync(update_type); }

const std::string& CacheUpdateTrait::Name() const { return impl_->Name(); }

AllowedUpdateTypes CacheUpdateTrait::GetAllowedUpdateTypes() const { return impl_->GetAllowedUpdateTypes(); }

void CacheUpdateTrait::EarlyStartPeriodicUpdates(utils::Flags<Flag> flags)
{
    impl_->StartPeriodicUpdates(flags);
}

void CacheUpdateTrait::EarlyStopPeriodicUpdates()
{
    impl_->StopPeriodicUpdates();
}

void CacheUpdateTrait::StartPeriodicUpdates() { impl_->StartPeriodicUpdates(GetStartFlags()); }

void CacheUpdateTrait::StopPeriodicUpdates() { impl_->StopPeriodicUpdates(); }

void CacheUpdateTrait::OnCacheModified() { impl_->OnCacheModified(); }

bool CacheUpdateTrait::HasPreAssignCheck() const { return impl_->HasPreAssignCheck(); }

bool CacheUpdateTrait::IsSafeDataLifetime() const { return impl_->IsSafeDataLifetime(); }

void CacheUpdateTrait::SetDataSizeStatistic(std::size_t size) noexcept { impl_->SetDataSizeStatistic(size); }

rcu::ReadablePtr<Config> CacheUpdateTrait::GetConfig() const { return impl_->GetConfig(); }

engine::TaskProcessor& CacheUpdateTrait::GetCacheTaskProcessor() const { return impl_->GetCacheTaskProcessor(); }

utils::Flags<CacheUpdateTrait::Flag> CacheUpdateTrait::GetStartFlags() const { return {}; }

void CacheUpdateTrait::MarkAsExpired() {}

void CacheUpdateTrait::GetAndWrite(dump::Writer&) const { dump::ThrowDumpUnimplemented(Name()); }

void CacheUpdateTrait::ReadAndSet(dump::Reader&) { dump::ThrowDumpUnimplemented(Name()); }

}  // namespace cache

USERVER_NAMESPACE_END
