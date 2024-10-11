#pragma once

#include <userver/ugrpc/server/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
auto Dispatch(void (ServiceBaseType::*service_method)(UnaryCall<ResponseType>&,
                                                      RequestType&&)) {
  return service_method;
}

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
auto Dispatch(void (ServiceBaseType::*service_method)(
    InputStream<RequestType, ResponseType>&)) {
  return service_method;
}

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
auto Dispatch(void (ServiceBaseType::*service_method)(
    OutputStream<ResponseType>&, RequestType&&)) {
  return service_method;
}

template <typename ServiceBaseType, typename RequestType, typename ResponseType>
auto Dispatch(void (ServiceBaseType::*service_method)(
    BidirectionalStream<RequestType, ResponseType>&)) {
  return service_method;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
