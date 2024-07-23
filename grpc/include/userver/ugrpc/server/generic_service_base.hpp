#pragma once

/// @file userver/ugrpc/server/generic_service_base.hpp
/// @brief @copybrief ugrpc::server::GenericServiceBase

#include <grpcpp/support/byte_buffer.h>

#include <userver/ugrpc/server/rpc.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Allows to handle RPCs with dynamic method names.
///
/// To use:
///
/// 1. inherit from this class in e.g. `YourGenericService`;
/// 2. put `YourGenericService` in a descendent of @ref ServiceComponentBase,
///    e.g. `YourGenericServiceComponent`
///
/// Alternatively, just inherit from @ref GenericServiceBase::Component, with
/// the disadvantage that the service will not be unit-testable.
///
/// The API is mainly intended for proxies, where the request-response body is
/// passed unchanged, with settings taken solely from the RPC metadata.
/// In cases where the code needs to operate on the actual messages,
/// serialization of requests and responses is left as an excercise to the user.
///
/// Middlewares are customizable and are applied as usual, except that no
/// message hooks are called, meaning that there won't be any logs of messages
/// from the default middleware.
///
/// Metrics are written per-method by default. If malicious clients initiate
/// RPCs with a lot of different garbage names, it may cause OOM due to
/// per-method metrics piling up.
/// TODO(TAXICOMMON-9108) allow limiting the amount of metrics.
///
/// Statically-typed services, if registered, take priority over generic
/// services. It only makes sense to register at most 1 generic service.
///
/// ## Example GenericServiceBase usage with known message types
///
/// @snippet grpc/tests/generic_server_test.cpp  sample
class GenericServiceBase {
 public:
  /// Inherits from both @ref GenericServiceBase and @ref ServiceComponentBase.
  /// Allows to implement the service directly in a component.
  /// The disadvantage is that such services are not unit-testable.
  using Component = impl::ServiceComponentBase<GenericServiceBase>;

  using Call = BidirectionalStream<grpc::ByteBuffer, grpc::ByteBuffer>;

  GenericServiceBase(GenericServiceBase&&) = delete;
  GenericServiceBase& operator=(GenericServiceBase&&) = delete;
  virtual ~GenericServiceBase();

  /// @brief Override this method in the derived class to handle all RPCs.
  /// RPC name can be obtained through @ref CallAnyBase::GetCallName.
  /// @note RPCs of all kinds (including unary RPCs) are represented using
  /// BidirectionalStream API.
  /// @note The implementation of the method should call `Finish` or
  /// `FinishWithError`, otherwise the server will respond with an "internal
  /// server error" status.
  virtual void Handle(Call& call) = 0;

 protected:
  GenericServiceBase() = default;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
