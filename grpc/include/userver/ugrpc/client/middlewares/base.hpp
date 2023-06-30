#pragma once

/// @file userver/ugrpc/client/middlewares/middleware_base.hpp
/// @brief @copybrief ugrpc::client::MiddlewareBase

#include <memory>
#include <vector>

#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/client/rpc.hpp>

#include <userver/components/loggable_component_base.hpp>

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
  MiddlewareCallContext(const Middlewares& middlewares, CallAnyBase& call,
                        std::function<void()> user_call,
                        const ::google::protobuf::Message* request)
      : middleware_(middlewares.begin()),
        middleware_end_(middlewares.end()),
        user_call_(std::move(user_call)),
        call_(call),
        request_(request) {}
  /// @endcond

  /// @brief Call next plugin, or gRPC handler if none
  void Next();

  /// @brief Get original gRPC Call
  CallAnyBase& GetCall();

  /// @brief Get initial gRPC request. For RPC w/o initial request
  /// returns nullptr.
  const ::google::protobuf::Message* GetInitialRequest();

 private:
  Middlewares::const_iterator middleware_;
  Middlewares::const_iterator middleware_end_;
  std::function<void()> user_call_;

  CallAnyBase& call_;
  const ::google::protobuf::Message* request_;
};

/// @ingroup userver_base_classes
///
/// @brief Base class for server gRPC middleware
class MiddlewareBase {
 public:
  MiddlewareBase();
  MiddlewareBase(const MiddlewareBase&) = delete;
  MiddlewareBase& operator=(const MiddlewareBase&) = delete;
  MiddlewareBase& operator=(MiddlewareBase&&) = delete;

  virtual ~MiddlewareBase();

  /// @brief Handles the gRPC request
  /// @note You MUST call context.Next() inside eventually, otherwise it will
  /// assert
  virtual void Handle(MiddlewareCallContext& context) const = 0;
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
class MiddlewareComponentBase : public components::LoggableComponentBase {
  using components::LoggableComponentBase::LoggableComponentBase;

 public:
  /// @brief Returns a middleware according to the component's settings
  virtual std::shared_ptr<const MiddlewareFactoryBase>
  GetMiddlewareFactory() = 0;
};

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
