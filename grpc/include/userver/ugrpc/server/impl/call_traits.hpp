#pragma once

#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/ugrpc/rpc_type.hpp>
#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/stream_adapter.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/ugrpc/server/stream.hpp>

namespace grpc {
class GenericServerContext;
}  // namespace grpc

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {
class CallContext;
class GenericCallContext;
}  // namespace ugrpc::server

namespace ugrpc::server::impl {

struct NoInitialRequest final {};

grpc::ServerContext DetectRawContextType(CallContext&);
grpc::GenericServerContext DetectRawContextType(GenericCallContext&);

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsUnaryCall final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncResponseWriter<Response>;
    using InitialRequest = Request;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = NoStreamingAdapter;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = Result<Response> (ServiceBase::*)(ContextType&, Request&&);
    static constexpr auto kRpcType = RpcType::kUnary;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsInputStream final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncReader<Response, Request>;
    using InitialRequest = NoInitialRequest;
    using RawContextType = ::grpc::ServerContext;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = ReaderAdapter<CallTraitsInputStream>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = Result<Response> (ServiceBase::*)(CallContext&, Reader<Request>&);
    static constexpr auto kRpcType = RpcType::kClientStreaming;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsOutputStream final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncWriter<Response>;
    using InitialRequest = Request;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = WriterAdapter<CallTraitsOutputStream>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = StreamingResult<Response> (ServiceBase::*)(CallContext&, Request&&, Writer<Response>&);
    static constexpr auto kRpcType = RpcType::kServerStreaming;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsBidirectionalStream final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = grpc::ServerAsyncReaderWriter<Response, Request>;
    using InitialRequest = NoInitialRequest;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = ReaderWriterAdapter<CallTraitsBidirectionalStream>;
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

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
