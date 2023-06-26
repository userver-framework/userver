#pragma once

/// @file userver/ugrpc/server/middlewares/middleware_base.hpp
/// @brief @copybrief ugrpc::server::MiddlewareBase

#include <memory>
#include <vector>

#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/server/rpc.hpp>

#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Context for middleware-specific data during gRPC call
class MiddlewareCallContext final {
 public:
  /// @cond
  MiddlewareCallContext(const Middlewares& middlewares, CallAnyBase& call,
                        std::function<void()> user_call,
                        std::string_view service_name,
                        std::string_view method_name,
                        const dynamic_config::Snapshot& config,
                        const ::google::protobuf::Message* request)
      : middleware_(middlewares.begin()),
        middleware_end_(middlewares.end()),
        user_call_(std::move(user_call)),
        call_(call),
        service_name_(service_name),
        method_name_(method_name),
        config_(config),
        request_(request) {}
  /// @endcond

  /// @brief Call next plugin, or gRPC handler if none
  void Next();

  /// @brief Get original gRPC Call
  CallAnyBase& GetCall();

  /// @brief Get name of gRPC service
  std::string_view GetServiceName() const;

  /// @brief Get name of called gRPC method
  std::string_view GetMethodName() const;

  /// @brief Get values extracted from dynamic_config. Snapshot will be
  /// deleted when the last meddleware completes
  const dynamic_config::Snapshot& GetInitialDynamicConfig() const;

  /// @brief Get initial gRPC request. For RPC w/o initial request
  /// returns nullptr.
  const ::google::protobuf::Message* GetInitialRequest();

 private:
  void ClearMiddlewaresResources();

  Middlewares::const_iterator middleware_;
  Middlewares::const_iterator middleware_end_;
  std::function<void()> user_call_;

  CallAnyBase& call_;

  std::string_view service_name_;
  std::string_view method_name_;
  std::optional<dynamic_config::Snapshot> config_;
  const ::google::protobuf::Message* request_;
};

/// @brief Base class for server gRPC middleware
class MiddlewareBase {
 public:
  MiddlewareBase();
  MiddlewareBase(const MiddlewareBase&) = delete;
  MiddlewareBase& operator=(const MiddlewareBase&) = delete;
  MiddlewareBase& operator=(MiddlewareBase&&) = delete;

  virtual ~MiddlewareBase();

  /// @brief Handles the gRPC request
  /// @note You should call context.Next() inside, otherwise the call will be
  /// dropped
  virtual void Handle(MiddlewareCallContext& context) const = 0;
};

/// @brief Base class for middleware component
class MiddlewareComponentBase : public components::LoggableComponentBase {
  using components::LoggableComponentBase::LoggableComponentBase;

 public:
  /// @brief Returns a middleware according to the component's settings
  virtual std::shared_ptr<MiddlewareBase> GetMiddleware() = 0;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
