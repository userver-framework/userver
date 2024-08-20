#pragma once

/// @file userver/ugrpc/client/middlewares/middleware_base.hpp
/// @brief @copybrief ugrpc::client::MiddlewareBase

#include <memory>
#include <vector>

#include <userver/components/component_base.hpp>

#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/client/rpc.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

/// @brief Context for middleware-specific data during gRPC call
///
/// It is created for each gRPC Call and it stores aux. data
/// used by middlewares. Each registered middleware is called by
/// `Middleware::Handle` with the context passed as an argument.
/// A middleware may access Call and initial request (if any) using the context.
class MiddlewareCallContext final {
 public:
  /// @cond
  explicit MiddlewareCallContext(impl::RpcData& data);
  /// @endcond

  /// @returns the `ClientContext` used for this RPC
  grpc::ClientContext& GetContext() noexcept;

  /// @returns client name
  std::string_view GetClientName() const noexcept;

  /// @returns RPC name
  std::string_view GetCallName() const noexcept;

  /// @returns RPC kind
  CallKind GetCallKind() const noexcept;

  /// @returns RPC span
  tracing::Span& GetSpan() noexcept;

  /// @cond
  // For internal use only
  impl::RpcData& GetData(ugrpc::impl::InternalTag);
  /// @endcond

 private:
  impl::RpcData& data_;
};

/// @ingroup userver_base_classes
///
/// @brief Base class for client gRPC middleware
class MiddlewareBase {
 public:
  virtual ~MiddlewareBase();

  MiddlewareBase(const MiddlewareBase&) = delete;

  MiddlewareBase& operator=(const MiddlewareBase&) = delete;
  MiddlewareBase& operator=(MiddlewareBase&&) = delete;

  // default implemented methods

  /// called before `StartCall`
  virtual void PreStartCall(MiddlewareCallContext&) const;

  /// called after Finish
  /// @note could be not called if `Finish` was not called (eg deadline or
  /// network problem)
  /// @see @ref RpcInterruptedError
  virtual void PostFinish(MiddlewareCallContext&, const grpc::Status&) const;

  /// called before send message
  /// @note not called for `GenericClient` messages
  virtual void PreSendMessage(MiddlewareCallContext&,
                              const google::protobuf::Message&) const;

  /// called after receive message
  /// @note not called for `GenericClient` messages
  virtual void PostRecvMessage(MiddlewareCallContext&,
                               const google::protobuf::Message&) const;

 protected:
  MiddlewareBase();
};

/// @ingroup userver_base_classes
///
/// @brief Factory that creates specific client middlewares for clients
class MiddlewareFactoryBase {
 public:
  virtual ~MiddlewareFactoryBase();

  virtual std::shared_ptr<const MiddlewareBase> GetMiddleware(
      std::string_view client_name) const = 0;
};

using MiddlewareFactories =
    std::vector<std::shared_ptr<const MiddlewareFactoryBase>>;

/// @ingroup userver_base_classes
///
/// @brief Base class for client middleware component
class MiddlewareComponentBase : public components::ComponentBase {
  using components::ComponentBase::ComponentBase;

 public:
  /// @brief Returns a middleware according to the component's settings
  virtual std::shared_ptr<const MiddlewareFactoryBase>
  GetMiddlewareFactory() = 0;
};

namespace impl {

Middlewares InstantiateMiddlewares(const MiddlewareFactories& factories,
                                   const std::string& client_name);

}  // namespace impl

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
