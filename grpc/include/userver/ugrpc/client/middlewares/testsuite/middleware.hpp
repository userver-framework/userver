#pragma once

/// @file userver/ugrpc/client/middlewares/testsuite/middleware.hpp
/// @brief @copybrief ugrpc::client::middlewares::testsuite::Middleware

#include <string>
#include <string_view>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::testsuite {

/// @brief middleware for gRPC client testsuite
class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(std::string_view client_name);

    void PreStartCall(MiddlewareCallContext&) const override;

    void PostFinish(MiddlewareCallContext&, const grpc::Status&) const override;

private:
    std::string client_name_;
};

}  // namespace ugrpc::client::middlewares::testsuite

USERVER_NAMESPACE_END
