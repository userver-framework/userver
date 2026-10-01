#pragma once

#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/byte_buffer.h>
#include <grpcpp/support/server_callback.h>

#include <userver/ugrpc/rpc_type.hpp>
#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/stream_adapter.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/ugrpc/server/stream.hpp>

namespace grpc {
class CallbackServerContext;
class GenericServerContext;
class GenericCallbackServerContext;
}  // namespace grpc

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {
class CallContext;
class GenericCallContext;
}  // namespace ugrpc::server

namespace ugrpc::server::impl {

struct NoSerializedInitialRequest final {};
inline constinit NoSerializedInitialRequest no_serialized_initial_request;

template <typename CallbackTraits>
class CallbackResponder;

struct NoFinalResponse final {};
inline constinit NoFinalResponse no_final_response;

grpc::ServerContext DetectRawContextType(CallContext&);
grpc::GenericServerContext DetectRawContextType(GenericCallContext&);

grpc::CallbackServerContext DetectRawCallbackContextType(CallContext&);
grpc::GenericCallbackServerContext DetectRawCallbackContextType(GenericCallContext&);

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsUnaryCall {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncResponseWriter<grpc::ByteBuffer>;
    using SerializedInitialRequest = grpc::ByteBuffer;
    using SerializedFinalResponse = grpc::ByteBuffer;
    using ReactorBase = grpc::ServerUnaryReactor;
    using CallbackMethodHandler = grpc::internal::CallbackUnaryHandler<grpc::ByteBuffer, grpc::ByteBuffer>;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    template <typename Traits>
    using StreamAdapterFor = NoStreamingAdapter;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = Result<Response> (ServiceBase::*)(ContextType&, Request&&);
    static constexpr auto kRpcType = RpcType::kUnary;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsInputStream {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncReader<grpc::ByteBuffer, grpc::ByteBuffer>;
    using SerializedInitialRequest = NoSerializedInitialRequest;
    using SerializedFinalResponse = grpc::ByteBuffer;
    using ReactorBase = grpc::ServerReadReactor<grpc::ByteBuffer>;
    using CallbackMethodHandler = grpc::internal::CallbackClientStreamingHandler<grpc::ByteBuffer, grpc::ByteBuffer>;
    using RawContextType = ::grpc::ServerContext;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    template <typename Traits>
    using StreamAdapterFor = ReaderAdapter<Traits>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = Result<Response> (ServiceBase::*)(CallContext&, Reader<Request>&);
    static constexpr auto kRpcType = RpcType::kClientStreaming;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsOutputStream {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncWriter<grpc::ByteBuffer>;
    using SerializedInitialRequest = grpc::ByteBuffer;
    using SerializedFinalResponse = NoFinalResponse;
    using ReactorBase = grpc::ServerWriteReactor<grpc::ByteBuffer>;
    using CallbackMethodHandler = grpc::internal::CallbackServerStreamingHandler<grpc::ByteBuffer, grpc::ByteBuffer>;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    template <typename Traits>
    using StreamAdapterFor = WriterAdapter<Traits>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = StreamingResult<Response> (ServiceBase::*)(CallContext&, Request&&, Writer<Response>&);
    static constexpr auto kRpcType = RpcType::kServerStreaming;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsBidirectionalStream {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncReaderWriter<grpc::ByteBuffer, grpc::ByteBuffer>;
    using SerializedInitialRequest = NoSerializedInitialRequest;
    using SerializedFinalResponse = NoFinalResponse;
    using ReactorBase = grpc::ServerBidiReactor<grpc::ByteBuffer, grpc::ByteBuffer>;
    using CallbackMethodHandler = grpc::internal::CallbackBidiHandler<grpc::ByteBuffer, grpc::ByteBuffer>;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    template <typename Traits>
    using StreamAdapterFor = ReaderWriterAdapter<Traits>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = StreamingResult<Response> (ServiceBase::*)(ContextType&, ReaderWriter<Request, Response>&);
    static constexpr auto kRpcType = RpcType::kBidiStreaming;
};

template <typename HandlerMethod>
struct CallTraitsImpl;

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsImpl<Result<ResponseType> (ServiceBaseType::*)(ContextType&, RequestType&&)> final {
    using type = CallTraitsUnaryCall<ServiceBaseType, ContextType, RequestType, ResponseType>;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsImpl<Result<ResponseType> (ServiceBaseType::*)(ContextType&, Reader<RequestType>&)> final {
    using type = CallTraitsInputStream<ServiceBaseType, ContextType, RequestType, ResponseType>;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsImpl<
    StreamingResult<ResponseType> (ServiceBaseType::*)(ContextType&, RequestType&&, Writer<ResponseType>&)>
    final {
    using type = CallTraitsOutputStream<ServiceBaseType, ContextType, RequestType, ResponseType>;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsImpl<
    StreamingResult<ResponseType> (ServiceBaseType::*)(ContextType&, ReaderWriter<RequestType, ResponseType>&)>
    final {
    using type = CallTraitsBidirectionalStream<ServiceBaseType, ContextType, RequestType, ResponseType>;
};

template <typename HandlerMethod>
using CallTraits = typename CallTraitsImpl<HandlerMethod>::type;

template <typename BaseCallTraits>
struct CallbackCallTraits final : BaseCallTraits {
    using RawContext = decltype(DetectRawCallbackContextType(std::declval<typename BaseCallTraits::Context&>()));
    using RawResponder = CallbackResponder<CallbackCallTraits>;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
