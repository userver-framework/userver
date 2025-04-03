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
    std::string_view component_name
) noexcept
    : impl_(impl), component_name_(component_name) {}

engine::TaskProcessor& ComponentContext::GetTaskProcessor(const std::string& name) const {
    return impl_.GetTaskProcessor(name);
}

std::string_view ComponentContext::GetComponentName() const { return component_name_; }

impl::ComponentContextImpl& ComponentContext::GetImpl(utils::impl::InternalTag) const { return impl_; }

const Manager& ComponentContext::GetManager(utils::impl::InternalTag) const { return impl_.GetManager(); }

bool ComponentContext::Contains(std::string_view name) const noexcept { return impl_.Contains(name); }

void ComponentContext::ThrowNonRegisteredComponent(std::string_view name, std::string_view type) const {
    impl_.ThrowNonRegisteredComponent(name, type);
}

void ComponentContext::ThrowComponentTypeMismatch(
    std::string_view name,
    std::string_view type,
    RawComponentBase* component
) const {
    impl_.ThrowComponentTypeMismatch(name, type, component);
}

RawComponentBase* ComponentContext::DoFindComponent(std::string_view name) const { return impl_.DoFindComponent(name); }

}  // namespace components

USERVER_NAMESPACE_END
