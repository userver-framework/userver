#pragma once

/// @file userver/ugrpc/server/service_base.hpp
/// @brief @copybrief ugrpc::server::ServiceBase

#include <boost/container/flat_map.hpp>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/ugrpc/server/call_context.hpp>
#include <userver/ugrpc/server/impl/service_worker.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace impl {
struct ServiceInternals;
}  // namespace impl

/// Per-service settings
struct ServiceConfig final {
    /// TaskProcessor to use for serving RPCs.
    engine::TaskProcessor& task_processor;

    /// Server middlewares to use for the gRPC service.
    Middlewares middlewares;

    /// map of "status_code": log_level items to override span log level for specific status codes
    /// see @ref ugrpc::kStatusCodesMap for available statuses
    boost::container::flat_map<grpc::StatusCode, logging::Level> status_codes_log_level;
};

/// @brief The type-erased base class for all gRPC service implementations
/// @note User-defined services should inherit from code-generated base service
/// classes, not from this class directly.
class ServiceBase {
public:
    using CallContext = ugrpc::server::CallContext;

    ServiceBase& operator=(ServiceBase&&) = delete;
    virtual ~ServiceBase();

    /// @cond
    // Creates a worker that forwards requests to this service.
    // The service must be destroyed after the worker.
    // For internal use only.
    virtual std::unique_ptr<impl::ServiceWorker> MakeWorker(impl::ServiceInternals&& internals) = 0;
    /// @endcond
};

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
