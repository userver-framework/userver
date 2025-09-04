#pragma once

#include <unordered_set>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::headers_propagator {

struct Settings final {
    /// Headers that will be skipped during propagation.
    std::unordered_set<std::string> skip_headers{};
};

/// @brief middleware for propagate headers from @ref server::request::GetPropagatedHeaders to the gRPC server.
///
/// Headers from @ref server::request::GetPropagatedHeaders and from static config ('skip-headers') are casted to
/// lowercase during comparison and then they are propagated in lowercase (the requirement of gRPC metadata).
///
/// Middleware propagates all headers in lowercase mode.
class Middleware final : public MiddlewareBase {
public:
    Middleware(Settings&&);

    void PreStartCall(MiddlewareCallContext&) const override;

private:
    Settings settings_;
};

}  // namespace ugrpc::client::middlewares::headers_propagator

USERVER_NAMESPACE_END
