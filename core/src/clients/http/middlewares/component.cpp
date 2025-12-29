#include <userver/clients/http/middlewares/component.hpp>

#include <userver/utils/impl/internal_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares {

ComponentBase::ComponentBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context,
    MiddlewareIndex index
)
    : components::ComponentBase(config, context),
      index_(index)
{}

MiddlewareIndex ComponentBase::GetIndex(utils::impl::InternalTag) const { return index_; }

}  // namespace clients::http::middlewares

USERVER_NAMESPACE_END
