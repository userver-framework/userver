#pragma once

/// @file userver/ugrpc/client/middlewares/testsuite/middleware.hpp
/// @brief @copybrief ugrpc::client::middlewares::testsuite::Middleware

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::testsuite {

/// @brief middleware for gRPC client testsuite
class Middleware final : public MiddlewareBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// ugrpc::client::middlewares::testsuite::Component.
    static constexpr std::string_view kName = "grpc-client-middleware-testsuite";

    /// @brief dependency of this middleware. PostCore group.
    static inline const auto kDependency = USERVER_NAMESPACE::middlewares::MiddlewareDependencyBuilder()
                                               .InGroup<USERVER_NAMESPACE::middlewares::groups::PostCore>();

    Middleware() = default;

    void PostFinish(MiddlewareCallContext&, const grpc::Status&) const override;
};

}  // namespace ugrpc::client::middlewares::testsuite

USERVER_NAMESPACE_END
