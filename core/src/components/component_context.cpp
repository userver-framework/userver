#include <userver/components/component_context.hpp>

#include <components/component_context_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentsLoadCancelledException::ComponentsLoadCancelledException()
    : std::runtime_error("Components load cancelled") {}

ComponentsLoadCancelledException::ComponentsLoadCancelledException(
    const std::string& message)
    : std::runtime_error(message) {}

ComponentContext::ComponentContext() noexcept = default;

void ComponentContext::Emplace(
    const Manager& manager,
    std::vector<std::string>&& loading_component_names) {
  impl_ = std::make_unique<Impl>(manager, std::move(loading_component_names));
}

void ComponentContext::Reset() noexcept { impl_.reset(); }

ComponentContext::~ComponentContext() = default;

impl::ComponentBase* ComponentContext::AddComponent(
    std::string_view name, const ComponentFactory& factory) {
  return impl_->AddComponent(name, factory, *this);
}

void ComponentContext::OnAllComponentsLoaded() {
  impl_->OnAllComponentsLoaded();
}

void ComponentContext::OnAllComponentsAreStopping() {
  impl_->OnAllComponentsAreStopping();
}

void ComponentContext::ClearComponents() { impl_->ClearComponents(); }

engine::TaskProcessor& ComponentContext::GetTaskProcessor(
    const std::string& name) const {
  return impl_->GetTaskProcessor(name);
}

const Manager& ComponentContext::GetManager() const {
  return impl_->GetManager();
}

void ComponentContext::CancelComponentsLoad() { impl_->CancelComponentsLoad(); }

bool ComponentContext::IsAnyComponentInFatalState() const {
  return impl_->IsAnyComponentInFatalState();
}

bool ComponentContext::Contains(std::string_view name) const noexcept {
  return impl_->Contains(name);
}

void ComponentContext::ThrowNonRegisteredComponent(
    std::string_view name, std::string_view type) const {
  impl_->ThrowNonRegisteredComponent(name, type);
}

void ComponentContext::ThrowComponentTypeMismatch(
    std::string_view name, std::string_view type,
    impl::ComponentBase* component) const {
  impl_->ThrowComponentTypeMismatch(name, type, component);
}

impl::ComponentBase* ComponentContext::DoFindComponent(
    std::string_view name) const {
  return impl_->DoFindComponent(name);
}

}  // namespace components

USERVER_NAMESPACE_END
