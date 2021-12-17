#pragma once

/// @file userver/ugrpc/server/service_base.hpp
/// @brief @copybrief ugrpc::server::ServiceBase

#include <userver/ugrpc/server/impl/service_worker.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

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
