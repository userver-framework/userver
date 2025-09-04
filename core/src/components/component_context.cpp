#include <userver/components/component_context.hpp>

#include <components/component_context_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentsLoadCancelledException::ComponentsLoadCancelledException()
    : std::runtime_error("Components load cancelled") {}

ComponentsLoadCancelledException::ComponentsLoadCancelledException(std::string_view message)
    : std::runtime_error(std::string{message}) {}

ComponentContext::ComponentContext(
    utils::impl::InternalTag,
    impl::ComponentContextImpl& impl,
    impl::ComponentInfo& component_info
) noexcept
    : impl_(impl), component_info_(component_info) {}

engine::TaskProcessor& ComponentContext::GetTaskProcessor(std::string_view name) const {
    return impl_.GetTaskProcessor(name);
}

std::string_view ComponentContext::GetComponentName() const { return component_info_.GetName(); }

impl::ComponentContextImpl& ComponentContext::GetImpl(utils::impl::InternalTag) const { return impl_; }

const impl::Manager& ComponentContext::GetManager(utils::impl::InternalTag) const { return impl_.GetManager(); }

bool ComponentContext::Contains(std::string_view name) const noexcept { return impl_.Contains(name); }

void ComponentContext::ThrowNonRegisteredComponent(std::string_view name, std::string_view type) const {
    impl_.ThrowNonRegisteredComponent(name, type, component_info_);
}

void ComponentContext::ThrowComponentTypeMismatch(
    std::string_view name,
    std::string_view type,
    RawComponentBase* component
) const {
    impl_.ThrowComponentTypeMismatch(name, type, component, component_info_);
}

RawComponentBase* ComponentContext::DoFindComponent(std::string_view name) const {
    return impl_.DoFindComponent(name, component_info_);
}

}  // namespace components

USERVER_NAMESPACE_END
