#pragma once

/// @file userver/ugrpc/server/service_base.hpp
/// @brief @copybrief ugrpc::server::ServiceBase

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/ugrpc/server/impl/service_worker.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

/// Per-service settings
struct ServiceConfig final {
  /// TaskProcessor to use for serving RPCs.
  engine::TaskProcessor& task_processor;

  /// Server middlewares names to use for the gRPC service.
  std::vector<std::string> middlewares_names;
};

/// @brief The type-erased base class for all gRPC service implementations
/// @note User-defined services should inherit from code-generated base service
/// classes, not from this class directly.
class ServiceBase {
 public:
  ServiceBase& operator=(ServiceBase&&) = delete;
  virtual ~ServiceBase();

  /// @cond
  // Creates a worker that forwards requests to this service.
  // The service must be destroyed after the worker.
  // For internal use only.
  virtual std::unique_ptr<impl::ServiceWorker> MakeWorker(
      impl::ServiceSettings&& settings) = 0;
  /// @endcond
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
