#pragma once

/// @file userver/ugrpc/server/reactor.hpp
/// @brief @copybrief ugrpc::server::Reactor

#include <memory>

#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/codegen/service_type.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// @brief Processes requests on a `CompletionQueue` using a handler
/// @note Must be destroyed after the corresponding `CompletionQueue`
class Reactor {
 public:
  Reactor& operator=(Reactor&&) = delete;
  virtual ~Reactor();

  /// Registers itself in the provided `ServerBuilder`
  virtual ::grpc::Service& GetService() = 0;

  /// Start serving requests
  virtual void Start() = 0;
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
