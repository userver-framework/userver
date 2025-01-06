#pragma once

#include <userver/ugrpc/server/call_context.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/ugrpc/server/rpc.hpp>
#include <userver/ugrpc/server/stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct NoInitialRequest final {};

enum class CallCategory {
    kUnary,
    kInputStream,
    kOutputStream,
    kBidirectionalStream,
    kGeneric,
};

template <typename HandlerMethod>
struct CallTraits;

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<ugrpc::server::Result<ResponseType> (ServiceBaseType::*)(ugrpc::server::CallContext&, RequestType&&)>
    final {
    using ServiceBase = ServiceBaseType;
    using Request = RequestType;
    using Response = ResponseType;
    using RawCall = impl::RawResponseWriter<ResponseType>;
    using InitialRequest = Request;
    using Call = UnaryCall<Response>;
    using ContextType = ::grpc::ServerContext;
    using ServiceMethod = ugrpc::server::Result<Response> (ServiceBase::*)(ugrpc::server::CallContext&, Request&&);
    static constexpr auto kCallCategory = CallCategory::kUnary;
};

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<ugrpc::server::Result<
    ResponseType> (ServiceBaseType::*)(ugrpc::server::CallContext&, ugrpc::server::Reader<RequestType>&)>
    final {
    using ServiceBase = ServiceBaseType;
    using Request = RequestType;
    using Response = ResponseType;
    using RawCall = impl::RawReader<Request, Response>;
    using InitialRequest = NoInitialRequest;
    using Call = InputStream<Request, Response>;
    using ContextType = ::grpc::ServerContext;
    using ServiceMethod =
        ugrpc::server::Result<Response> (ServiceBase::*)(ugrpc::server::CallContext&, ugrpc::server::Reader<Request>&);
    static constexpr auto kCallCategory = CallCategory::kInputStream;
};

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<ugrpc::server::StreamingResult<
    ResponseType> (ServiceBaseType::*)(ugrpc::server::CallContext&, RequestType&&, ugrpc::server::Writer<ResponseType>&)>
    final {
    using ServiceBase = ServiceBaseType;
    using Request = RequestType;
    using Response = ResponseType;
    using RawCall = impl::RawWriter<Response>;
    using InitialRequest = Request;
    using Call = OutputStream<Response>;
    using ContextType = ::grpc::ServerContext;
    using ServiceMethod = ugrpc::server::StreamingResult<
        Response> (ServiceBase::*)(ugrpc::server::CallContext&, Request&&, ugrpc::server::Writer<Response>&);
    static constexpr auto kCallCategory = CallCategory::kOutputStream;
};

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<ugrpc::server::StreamingResult<
    ResponseType> (ServiceBaseType::*)(ugrpc::server::CallContext&, ugrpc::server::ReaderWriter<RequestType, ResponseType>&)>
    final {
    using ServiceBase = ServiceBaseType;
    using Request = RequestType;
    using Response = ResponseType;
    using RawCall = impl::RawReaderWriter<Request, Response>;
    using InitialRequest = NoInitialRequest;
    using Call = BidirectionalStream<Request, Response>;
    using ContextType = ::grpc::ServerContext;
    using ServiceMethod = ugrpc::server::StreamingResult<
        Response> (ServiceBase::*)(ugrpc::server::CallContext&, ugrpc::server::ReaderWriter<Request, Response>&);
    static constexpr auto kCallCategory = CallCategory::kBidirectionalStream;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
