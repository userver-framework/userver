#pragma once

/// @file userver/ugrpc/server/generic_service_base.hpp
/// @brief @copybrief ugrpc::server::GenericServiceBase

#include <grpcpp/support/byte_buffer.h>

#include <userver/ugrpc/server/call_context.hpp>
#include <userver/ugrpc/server/result.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>
#include <userver/ugrpc/server/stream.hpp>

// use by legacy
#include <userver/ugrpc/server/rpc.hpp>

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
/// serialization of requests and responses is left as an exercise to the user.
///
/// Middlewares are customizable and are applied as usual, except that no
/// message hooks are called, meaning that there won't be any logs of messages
/// from the default middleware.
///
/// Statically-typed services, if registered, take priority over generic
/// services. It only makes sense to register at most 1 generic service.
///
/// ## Generic gRPC service metrics
///
/// Metrics are accounted for `"Generic/Generic"` fake call name by default.
/// This is the safe choice that avoids potential OOMs.
/// To use the real dynamic RPC name for metrics, use
/// @ref CallAnyBase::SetMetricsCallName in conjunction with
/// @ref CallAnyBase::GetCallName.
///
/// @warning If the microservice serves as a proxy and has untrusted clients, it
/// is a good idea to avoid having per-RPC metrics to avoid the
/// situations where an upstream client can spam various RPCs with non-existent
/// names, which leads to this microservice spamming RPCs with non-existent
/// names, which leads to creating storage for infinite metrics and causes OOM.
///
/// ## Example GenericServiceBase usage with known message types
///
/// @snippet grpc/tests/generic_server_test.cpp  sample
///
/// For a more complete sample, see @ref grpc_generic_api.
class GenericServiceBase {
 public:
  /// Inherits from both @ref GenericServiceBase and @ref ServiceComponentBase.
  /// Allows to implement the service directly in a component.
  /// The disadvantage is that such services are not unit-testable.
  using Component = impl::ServiceComponentBase<GenericServiceBase>;

  using GenericCallContext = ugrpc::server::GenericCallContext;

  using GenericReaderWriter =
      ugrpc::server::ReaderWriter<grpc::ByteBuffer, grpc::ByteBuffer>;
  using GenericResult = ugrpc::server::StreamingResult<grpc::ByteBuffer>;

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
  virtual GenericResult Handle(GenericCallContext& context,
                               GenericReaderWriter& stream);

  // Legacy
  using Call = BidirectionalStream<grpc::ByteBuffer, grpc::ByteBuffer>;
  [[deprecated(
      "Use 'grpc::Status Handle(GenericCallContext&, "
      "GenericReaderWriter&)'")]] virtual void
  Handle(Call& call);

 protected:
  GenericServiceBase() = default;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
