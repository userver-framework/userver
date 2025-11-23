#pragma once

#include <string>

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::origin {

struct Settings final {
    /// The name of the current deploy unit that will appear in `x-origin` metadata.
    /// If `std::nullopt`, then the middleware effectively does nothing.
    std::optional<std::string> user_agent;
};

/// @brief gRPC client middleware that sets `x-origin` metadata. @ref ugrpc::server::middlewares::log::Component will
/// then copy that to `useragent` tag in server request log.
///
/// @see @ref ugrpc::client::middlewares::origin::Component
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(Settings&&);

    void PreStartCall(MiddlewareCallContext&) const override;

private:
    Settings settings_;
};

}  // namespace ugrpc::client::middlewares::origin

USERVER_NAMESPACE_END
