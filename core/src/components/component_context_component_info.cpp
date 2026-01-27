#include "component_context_component_info.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/tracer.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

namespace {

constexpr utils::StringLiteral kComponentName = "component_name";
constexpr utils::StringLiteral kStopComponentRootName = "component_stop";
constexpr utils::StringLiteral kOnAllComponentsAreStopping = "on_all_components_are_stopping";

template <typename Range>
std::string JoinNamesFromInfoImpl(const Range& component_info_refs, std::string_view separator) {
    return fmt::to_string(fmt::join(
        component_info_refs | boost::adaptors::transformed([](auto info) { return info->GetName(); }),
        separator
    ));
}

}  // namespace

StageSwitchingCancelledException::StageSwitchingCancelledException(const std::string& message)
    : std::runtime_error(message) {}

ComponentInfo::ComponentInfo(std::string name)
    : name_(std::move(name))
{}

void ComponentInfo::SetComponent(std::unique_ptr<RawComponentBase>&& component) {
    bool call_on_loading_cancelled = false;
    {
        // Calling user callback under no lock
        AfterConstruction();

        const std::lock_guard lock{mutex_};
        component_ = std::move(component);
        stage_ = ComponentLifetimeStage::kCreated;
        if (stage_switching_cancelled_) {
            call_on_loading_cancelled = true;
        }
    }

    if (call_on_loading_cancelled) {
        OnLoadingCancelled();
    }
    cv_.NotifyAll();
}

void ComponentInfo::AfterConstruction()
{
    scope_registration_finished_ = true;

    // A tweak to be sure in case of parial initialization only
    // already initialized scopes' before_dtr() are called
    auto tmp_resource_scopes = std::move(resource_scopes_);
    resource_scopes_.clear();  // for clang-static-analyzer
    for (auto& resource_scope : tmp_resource_scopes) {
        resource_scope->AfterConstruction();

        resource_scopes_.push_back(std::move(resource_scope));
    }
}

void ComponentInfo::ClearComponent() {
    if (!HasComponent()) {
        return;
    }
    tracing::Span span(std::string{kStopComponentRootName});
    span.AddTag(std::string{kComponentName}, name_);

    auto component = ExtractComponent();
    LOG_DEBUG() << "Stopping component";

    // Call Scopes' pre-destruction callbacks in reverse order
    for (auto& scope : resource_scopes_ | boost::adaptors::reversed) {
        scope.reset();
    }
    resource_scopes_.clear();

    component.reset();
    LOG_DEBUG() << "Stopped component";
}

RawComponentBase* ComponentInfo::GetComponent() const {
    const std::lock_guard lock{mutex_};
    return component_.get();
}

RawComponentBase* ComponentInfo::WaitAndGetComponent() const {
    std::unique_lock lock{mutex_};
    auto ok = cv_.Wait(lock, [this]() { return stage_switching_cancelled_ || component_ != nullptr; });
    if (!ok || stage_switching_cancelled_) {
        throw ComponentsLoadCancelledException();
    }
    return component_.get();
}

void ComponentInfo::AddItDependsOn(ComponentInfo& component) {
    const std::lock_guard lock{mutex_};
    UASSERT_MSG(
        stage_ == ComponentLifetimeStage::kNull,
        "Do not use context.FindComponent() or context.FindComponentOptional() "
        "outside of the component constructor."
    );
    it_depends_on_.insert(component);
}

void ComponentInfo::AddDependsOnIt(ComponentInfo& component) {
    const std::lock_guard lock{mutex_};
    UASSERT_MSG(
        stage_ == ComponentLifetimeStage::kNull || stage_ == ComponentLifetimeStage::kCreated,
        "Do not use context.FindComponent() or context.FindComponentOptional() "
        "outside of the component constructor."
    );
    depends_on_it_.insert(component);
}

bool ComponentInfo::CheckItDependsOn(ComponentInfo& component) const {
    std::unique_lock lock{mutex_};
    if (stage_ != ComponentLifetimeStage::kNull) {
        lock.unlock();
    }

    return it_depends_on_.find(component) != it_depends_on_.end();
}

bool ComponentInfo::CheckDependsOnIt(ComponentInfo& component) const {
    std::unique_lock lock{mutex_};
    if (stage_ != ComponentLifetimeStage::kNull && stage_ != ComponentLifetimeStage::kCreated) {
        lock.unlock();
    }

    return depends_on_it_.find(component) != depends_on_it_.end();
}

void ComponentInfo::SetStageSwitchingCancelled(bool cancelled) {
    {
        const std::lock_guard lock{mutex_};
        stage_switching_cancelled_ = cancelled;
    }
    cv_.NotifyAll();
}

void ComponentInfo::OnLoadingCancelled() {
    if (!HasComponent()) {
        return;
    }
    if (on_loading_cancelled_called_.exchange(true)) {
        return;
    }
    component_->OnLoadingCancelled();
}

void ComponentInfo::OnAllComponentsLoaded() {
    if (!HasComponent()) {
        return;
    }
    try {
        component_->OnAllComponentsLoaded();
    } catch (const std::exception& ex) {
        const std::string message = "OnAllComponentsLoaded() failed for component " + name_ + ": " + ex.what();
        LOG_ERROR() << message;
        throw std::runtime_error(message);
    }
}

void ComponentInfo::OnAllComponentsAreStopping() {
    if (!HasComponent()) {
        return;
    }
    try {
        const tracing::Span span(std::string{kOnAllComponentsAreStopping});
        component_->OnAllComponentsAreStopping();
    } catch (const std::exception& ex) {
        LOG_ERROR() << "OnAllComponentsAreStopping() failed for component " << name_ << ": " << ex;
    }
}

void ComponentInfo::SetStage(ComponentLifetimeStage stage) {
    {
        const std::lock_guard lock{mutex_};
        stage_ = stage;
    }
    cv_.NotifyAll();
}

ComponentLifetimeStage ComponentInfo::GetStage() const {
    const std::lock_guard lock{mutex_};
    return stage_;
}

void ComponentInfo::WaitStage(ComponentLifetimeStage stage, std::string method_name) const {
    std::unique_lock lock{mutex_};
    auto ok = cv_.Wait(lock, [this, stage]() { return stage_switching_cancelled_ || stage_ == stage; });
    if (!ok && stage_switching_cancelled_) {
        throw StageSwitchingCancelledException(method_name.append(" cancelled"));
    }
}

std::string ComponentInfo::GetDependencies() const {
    if (it_depends_on_.empty()) {
        return {};
    }

    auto delimiter = fmt::format(R"("; "{}" -> ")", name_);
    return fmt::format(R"("{}" -> "{}" )", name_, JoinNamesFromInfo(it_depends_on_, delimiter));
}

void ComponentInfo::RegisterScope(ScopePtr resource_scope)
{
    UINVARIANT(!scope_registration_finished_, "Scope registration is available only in component constructor");
    resource_scopes_.push_back(std::move(resource_scope));
}

bool ComponentInfo::HasComponent() const {
    const std::lock_guard lock{mutex_};
    return !!component_;
}

std::unique_ptr<RawComponentBase> ComponentInfo::ExtractComponent() {
    std::unique_ptr<RawComponentBase> component;
    {
        const std::lock_guard lock{mutex_};
        std::swap(component, component_);
    }
    return component;
}

std::unordered_set<std::string> ExtractNamesFromInfo(const std::vector<ConstComponentInfoRef>& container) {
    auto v = container | boost::adaptors::transformed([](auto info) { return std::string{info->GetName()}; });
    return boost::copy_range<std::unordered_set<std::string>>(v);
}

std::string JoinNamesFromInfo(const std::vector<ConstComponentInfoRef>& container, std::string_view separator) {
    return JoinNamesFromInfoImpl(container, separator);
}

std::string JoinNamesFromInfo(const std::set<ComponentInfoRef>& container, std::string_view separator) {
    return JoinNamesFromInfoImpl(container, separator);
}

}  // namespace components::impl

USERVER_NAMESPACE_END
