#pragma once

/// @file userver/ugrpc/server/call_context.hpp
/// @brief @copybrief ugrpc::server::CallContext

#include <grpcpp/server_context.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

class CallAnyBase;

/// @brief gRPC call context
class CallContext {
 public:
  /// @cond
  explicit CallContext(CallAnyBase& call);
  /// @endcond

  /// @returns the `ServerContext` used for this RPC
  grpc::ServerContext& GetServerContext();

  /// @brief Name of the RPC in the format `full.path.ServiceName/MethodName`
  std::string_view GetCallName() const;

  /// @brief Get name of gRPC service
  std::string_view GetServiceName() const;

  /// @brief Get name of called gRPC method
  std::string_view GetMethodName() const;

 protected:
  /// @cond
  const CallAnyBase& GetCall() const;

  CallAnyBase& GetCall();
  /// @endcond

 private:
  CallAnyBase& call_;
};

/// @brief generic gRPC call context
class GenericCallContext : public CallContext {
 public:
  /// @cond
  using CallContext::CallContext;
  /// @endcond

  /// @brief Set a custom call name for metric labels
  void SetMetricsCallName(std::string_view call_name);
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
