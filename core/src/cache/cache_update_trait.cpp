#include <userver/cache/cache_update_trait.hpp>

#include <utility>

#include <cache/cache_dependencies.hpp>
#include <cache/cache_update_trait_impl.hpp>
#include <userver/dump/helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

CacheUpdateTrait::CacheUpdateTrait(const components::ComponentConfig& config,
                                   const components::ComponentContext& context)
    : CacheUpdateTrait(CacheDependencies::Make(config, context)) {}

CacheUpdateTrait::CacheUpdateTrait(CacheDependencies&& dependencies)
    : impl_(std::make_unique<Impl>(std::move(dependencies), *this)) {}

CacheUpdateTrait::~CacheUpdateTrait() = default;

void CacheUpdateTrait::Update(UpdateType update_type) {
  impl_->Update(update_type);
}

const std::string& CacheUpdateTrait::Name() const { return impl_->Name(); }

AllowedUpdateTypes CacheUpdateTrait::GetAllowedUpdateTypes() const {
  return impl_->GetAllowedUpdateTypes();
}

void CacheUpdateTrait::StartPeriodicUpdates(utils::Flags<Flag> flags) {
  impl_->StartPeriodicUpdates(flags);
}

void CacheUpdateTrait::StopPeriodicUpdates() { impl_->StopPeriodicUpdates(); }

void CacheUpdateTrait::AssertPeriodicUpdateStarted() {
  impl_->AssertPeriodicUpdateStarted();
}

void CacheUpdateTrait::OnCacheModified() { impl_->OnCacheModified(); }

bool CacheUpdateTrait::HasPreAssignCheck() const {
  return impl_->HasPreAssignCheck();
}

rcu::ReadablePtr<Config> CacheUpdateTrait::GetConfig() const {
  return impl_->GetConfig();
}

engine::TaskProcessor& CacheUpdateTrait::GetCacheTaskProcessor() const {
  return impl_->GetCacheTaskProcessor();
}

void CacheUpdateTrait::MarkAsExpired() {}

void CacheUpdateTrait::GetAndWrite(dump::Writer&) const {
  dump::ThrowDumpUnimplemented(Name());
}

void CacheUpdateTrait::ReadAndSet(dump::Reader&) {
  dump::ThrowDumpUnimplemented(Name());
}

}  // namespace cache

USERVER_NAMESPACE_END
