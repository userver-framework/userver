#pragma once

#include <userver/ugrpc/server/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct NoInitialRequest final {};

enum class CallCategory {
  kUnary,
  kInputStream,
  kOutputStream,
  kBidirectionalStream,
};

template <typename HandlerMethod>
struct CallTraits;

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<void (ServiceBaseType::*)(UnaryCall<ResponseType>&,
                                            RequestType&&)>
    final {
  using ServiceBase = ServiceBaseType;
  using Request = RequestType;
  using Response = ResponseType;
  using RawCall = impl::RawResponseWriter<ResponseType>;
  using InitialRequest = Request;
  using Call = UnaryCall<Response>;
  using ServiceMethod = void (ServiceBase::*)(Call&, Request&&);
  static constexpr auto kCallCategory = CallCategory::kUnary;
};

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<void (ServiceBaseType::*)(
    InputStream<RequestType, ResponseType>&)>
    final {
  using ServiceBase = ServiceBaseType;
  using Request = RequestType;
  using Response = ResponseType;
  using RawCall = impl::RawReader<Request, Response>;
  using InitialRequest = NoInitialRequest;
  using Call = InputStream<Request, Response>;
  using ServiceMethod = void (ServiceBase::*)(Call&);
  static constexpr auto kCallCategory = CallCategory::kInputStream;
};

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<void (ServiceBaseType::*)(OutputStream<ResponseType>&,
                                            RequestType&&)>
    final {
  using ServiceBase = ServiceBaseType;
  using Request = RequestType;
  using Response = ResponseType;
  using RawCall = impl::RawWriter<Response>;
  using InitialRequest = Request;
  using Call = OutputStream<Response>;
  using ServiceMethod = void (ServiceBase::*)(Call&, Request&&);
  static constexpr auto kCallCategory = CallCategory::kOutputStream;
};

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
struct CallTraits<void (ServiceBaseType::*)(
    BidirectionalStream<RequestType, ResponseType>&)>
    final {
  using ServiceBase = ServiceBaseType;
  using Request = RequestType;
  using Response = ResponseType;
  using RawCall = impl::RawReaderWriter<Request, Response>;
  using InitialRequest = NoInitialRequest;
  using Call = BidirectionalStream<Request, Response>;
  using ServiceMethod = void (ServiceBase::*)(Call&);
  static constexpr auto kCallCategory = CallCategory::kBidirectionalStream;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
