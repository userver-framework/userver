#pragma once

/// @file
/// @brief @copybrief grpc_proto_structs::client::StreamReadFuture

#include <userver/proto-structs/convert.hpp>
#include <userver/ugrpc/client/stream_read_future.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_proto_structs::client {

/// @brief proto-struct stream response future.
template <typename StructsResponse>
class StreamReadFuture final {
public:
    using ProtobufResponse = proto_structs::traits::CompatibleMessageType<StructsResponse>;
    using ProtobufStreamFuture = ugrpc::client::StreamReadFuture<ProtobufResponse>;

    StreamReadFuture(ProtobufStreamFuture&& future, ProtobufResponse& response)
        : future_{std::move(future)},
          response_(response)
    {}

    /// @brief Checks if the asynchronous call has completed
    ///        Note, that once user gets result, IsReady should not be called
    /// @return true if result ready
    [[nodiscard]] bool IsReady() const noexcept { return future_.IsReady(); }

    /// @brief Await response
    ///
    /// Upon completion the result is available in `response` that was
    /// specified when initiating the asynchronous read
    ///
    /// `Get` should not be called multiple times for the same StreamReadFuture.
    ///
    /// @throws ugrpc::client::RpcError on an RPC error
    /// @throws ugrpc::client::RpcCancelledError on task cancellation
    std::optional<StructsResponse> Get() {
        if (future_.Get()) {
            StructsResponse response;
            proto_structs::MessageToStruct(response_, response);
            return {response};
        }
        return std::nullopt;
    }

private:
    ProtobufStreamFuture future_;
    ProtobufResponse& response_;
};

}  // namespace grpc_proto_structs::client

USERVER_NAMESPACE_END
