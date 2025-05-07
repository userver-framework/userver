#pragma once

#include <userver/ugrpc/server/impl/async_methods.hpp>
#include <userver/ugrpc/server/impl/call_kind.hpp>
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
    using RawResponder = impl::RawResponseWriter<ResponseType>;
    using InitialRequest = Request;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = NoStreamingAdapter;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = Result<Response> (ServiceBase::*)(ContextType&, Request&&);
    static constexpr auto kCallKind = CallKind::kUnaryCall;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsInputStream final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = impl::RawReader<Request, Response>;
    using InitialRequest = NoInitialRequest;
    using RawContextType = ::grpc::ServerContext;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = ReaderAdapter<CallTraitsInputStream>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = Result<Response> (ServiceBase::*)(CallContext&, Reader<Request>&);
    static constexpr auto kCallKind = CallKind::kInputStream;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsOutputStream final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = impl::RawWriter<Response>;
    using InitialRequest = Request;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = WriterAdapter<CallTraitsOutputStream>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = StreamingResult<Response> (ServiceBase::*)(CallContext&, Request&&, Writer<Response>&);
    static constexpr auto kCallKind = CallKind::kOutputStream;
};

template <typename ServiceBaseType, typename ContextType, typename RequestType, typename ResponseType>
struct CallTraitsBidirectionalStream final {
    using Request = RequestType;
    using Response = ResponseType;
    using RawResponder = impl::RawReaderWriter<Request, Response>;
    using InitialRequest = NoInitialRequest;
    using Context = ContextType;
    using RawContext = decltype(DetectRawContextType(std::declval<ContextType&>()));
    using StreamAdapter = ReaderWriterAdapter<CallTraitsBidirectionalStream>;
    using ServiceBase = ServiceBaseType;
    using ServiceMethod = StreamingResult<Response> (ServiceBase::*)(ContextType&, ReaderWriter<Request, Response>&);
    static constexpr auto kCallKind = CallKind::kBidirectionalStream;
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
