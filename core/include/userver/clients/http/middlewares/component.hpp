#pragma once

#include <cstdint>

#include <userver/clients/http/middlewares/base.hpp>
#include <userver/components/component_base.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares {

using MiddlewareIndex = utils::StrongTypedef<class MiddlewareIndexTag, std::uint32_t>;

class ComponentBase : public components::ComponentBase {
public:
    virtual MiddlewareBase& GetMiddleware() = 0;

    /// @cond
    // For internal use only.
    MiddlewareIndex GetIndex(utils::impl::InternalTag) const;
    /// @endcond
protected:
    ComponentBase(
        const components::ComponentConfig& config,
        const components::ComponentContext& context,
        MiddlewareIndex index
    );

private:
    MiddlewareIndex index_;
};

}  // namespace clients::http::middlewares

USERVER_NAMESPACE_END
