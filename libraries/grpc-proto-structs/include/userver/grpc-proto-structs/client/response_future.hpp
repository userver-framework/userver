#pragma once

/// @file
/// @brief Structs future wrapper over Vanilla ResponseFufure.

#include <userver/proto-structs/convert.hpp>
#include <userver/ugrpc/client/response_future.hpp>
#include <userver/ugrpc/client/stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_proto_structs::client {

/// @brief proto-struct response future.
template <typename Response>
class ResponseFuture final {
public:
    using VanillaResponse = proto_structs::traits::CompatibleMessageType<Response>;
    using VanillaFuture = ugrpc::client::ResponseFuture<VanillaResponse>;

    explicit ResponseFuture(VanillaFuture&& future)
        : future_{std::move(future)}
    {}

    /// @brief Checks if the asynchronous call has completed.
    ///        Note, that once user gets result, IsReady should not be called.
    /// @return true if result ready.
    [[nodiscard]] bool IsReady() const { return future_.IsReady(); }

    /// @brief Await response until specified timepoint.
    ///
    /// @throws ugrpc::client::RpcError on an RPC error.
    [[nodiscard]] engine::FutureStatus WaitUntil(engine::Deadline deadline) const noexcept {
        return future_.WaitUntil(deadline);
    }

    /// @brief Await and read the response.
    ///
    /// `Get` should not be called multiple times for the same UnaryFuture.
    ///
    /// The connection is not closed, it will be reused for new RPCs.
    ///
    /// @returns the response on success.
    /// @throws ugrpc::client::RpcError on an RPC error.
    /// @throws ugrpc::client::RpcCancelledError on task cancellation.
    Response Get() { return proto_structs::MessageToStruct<Response>(future_.Get()); }

    /// @brief Cancel call.
    void Cancel() { return future_.Cancel(); }

    /// @brief Get call context, useful e.g. for accessing metadata.
    ugrpc::client::CallContext& GetContext() { return future_.GetContext(); }

    /// @overload
    const ugrpc::client::CallContext& GetContext() const { return future_.GetContext(); }

private:
    VanillaFuture future_;
};

}  // namespace grpc_proto_structs::client

USERVER_NAMESPACE_END
