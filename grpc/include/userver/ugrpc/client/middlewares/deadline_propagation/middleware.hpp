#pragma once

/// @file userver/ugrpc/client/middlewares/deadline_propagation/middleware.hpp
/// @brief @copybrief ugrpc::client::middlewares::deadline_propagation::Middleware

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::deadline_propagation {

/// @brief middleware for RPC handler logging settings
class Middleware final : public MiddlewareBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::client::middlewares::deadline_propagation::Component.
    static constexpr std::string_view kName = "grpc-client-deadline-propagation";

    /// @brief dependency of this middleware. Core group.
    static inline const auto kDependency = USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
                                               .InGroup<USERVER_NAMESPACE::middlewares::groups::Core>();

    Middleware() = default;

    void PreStartCall(MiddlewareCallContext& context) const override;
};

}  // namespace ugrpc::client::middlewares::deadline_propagation

USERVER_NAMESPACE_END
