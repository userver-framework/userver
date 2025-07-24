#pragma once

#include <string_view>
#include <type_traits>

#include <google/protobuf/message.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

/// @{
/// @brief Helper type aliases for low-level asynchronous gRPC streams
/// @see <grpcpp/impl/codegen/async_unary_call_impl.h>
/// @see <grpcpp/impl/codegen/async_stream_impl.h>
template <typename Response>
using RawResponseReader = std::unique_ptr<grpc::ClientAsyncResponseReader<Response>>;

template <typename Response>
using RawReader = std::unique_ptr<grpc::ClientAsyncReader<Response>>;

template <typename Request>
using RawWriter = std::unique_ptr<grpc::ClientAsyncWriter<Request>>;

template <typename Request, typename Response>
using RawReaderWriter = std::unique_ptr<grpc::ClientAsyncReaderWriter<Request, Response>>;
/// @}

template <typename Message>
const google::protobuf::Message* ToBaseMessage(const Message* message) {
    if constexpr (std::is_base_of_v<google::protobuf::Message, Message>) {
        return message;
    } else {
        return nullptr;
    }
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitAndTryCancelIfNeeded(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    engine::Deadline deadline,
    grpc::ClientContext& context
) noexcept;

ugrpc::impl::AsyncMethodInvocation::WaitStatus
WaitAndTryCancelIfNeeded(ugrpc::impl::AsyncMethodInvocation& invocation, grpc::ClientContext& context) noexcept;

void ProcessFinish(CallState& state, const google::protobuf::Message* final_response);

void ProcessFinishAbandoned(CallState& state) noexcept;

void CheckFinishStatus(CallState& state);

void ProcessCancelled(CallState& state, std::string_view stage) noexcept;

void ProcessNetworkError(CallState& state, std::string_view stage) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
