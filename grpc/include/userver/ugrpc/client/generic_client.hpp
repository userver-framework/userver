#pragma once

/// @file userver/ugrpc/client/generic_client.hpp
/// @brief @copybrief ugrpc::client::GenericClient

#include <optional>
#include <string_view>

#include <grpcpp/support/byte_buffer.h>

#include <userver/ugrpc/client/call_options.hpp>
#include <userver/ugrpc/client/generic_options.hpp>
#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/response_future.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @ingroup userver_clients
///
/// @brief Allows to talk to gRPC services (generic and normal) using dynamic
/// method names.
///
/// Created using @ref ugrpc::client::ClientFactory::MakeClient.
///
/// `call_name` must be in the format `full.path.to.TheService/MethodName`.
/// Note that unlike in base grpc++, there must be no initial `/` character.
///
/// The API is mainly intended for proxies, where the request-response body is
/// passed unchanged, with settings taken solely from the RPC metadata.
/// In cases where the code needs to operate on the actual messages,
/// serialization of requests and responses is left as an exercise to the user.
///
/// Middlewares are customizable and are applied as usual, except that no
/// message hooks are called, meaning that there won't be any logs of messages
/// from the default middleware.
///
/// There are no per-call-name metrics by default,
/// for details see @ref GenericOptions::metrics_call_name.
///
/// ## Example GenericClient usage with known message types
///
/// @snippet grpc/tests/generic_client_test.cpp  sample
///
/// For a more complete sample, see @ref grpc_generic_api.
class GenericClient final {
public:
    GenericClient(GenericClient&&) noexcept = default;
    GenericClient& operator=(GenericClient&&) noexcept = delete;

    /// Initiate a `single request -> single response` RPC with the given name.
    ResponseFuture<grpc::ByteBuffer> AsyncUnaryCall(
        std::string_view call_name,
        const grpc::ByteBuffer& request,
        CallOptions call_options = {},
        GenericOptions generic_options = {}
    ) const;

    /// Initiate a `single request -> single response` RPC with the given name.
    grpc::ByteBuffer UnaryCall(
        std::string_view call_name,
        const grpc::ByteBuffer& request,
        CallOptions call_options = {},
        GenericOptions generic_options = {}
    ) const;

    /// @cond
    // For internal use only.
    explicit GenericClient(impl::ClientInternals&&);

    static std::optional<ugrpc::impl::StaticServiceMetadata> GetMetadata() { return std::nullopt; }
    /// @endcond

private:
    template <typename Client>
    friend impl::ClientData& impl::GetClientData(Client& client);

    impl::ClientData impl_;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
