#pragma once

#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

#include <grpcpp/completion_queue.h>
#include <grpcpp/impl/service_type.h>

#include <userver/dynamic_config/source.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utils/statistics/fwd.hpp>

#include <userver/ugrpc/impl/static_metadata.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>
#include <userver/ugrpc/server/middlewares/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// Config for a `ServiceWorker`, provided by `ugrpc::server::Server`
struct ServiceSettings final {
  grpc::ServerCompletionQueue& queue;
  engine::TaskProcessor& task_processor;
  ugrpc::impl::StatisticsStorage& statistics_storage;
  const Middlewares& middlewares;
  const logging::LoggerPtr access_tskv_logger;
  const dynamic_config::Source config_source;
};

/// @brief Listens to requests for a gRPC service, forwarding them to a
/// user-provided service implementation. ServiceWorker instances are
/// created and owned by `Server`; services, on the other hand, are created
/// and owned by the user.
/// @note Must be destroyed after the corresponding `CompletionQueue`
class ServiceWorker {
 public:
  ServiceWorker& operator=(ServiceWorker&&) = delete;
  virtual ~ServiceWorker();

  /// Get the grpcpp service for registration in the `ServerBuilder`
  virtual grpc::Service& GetService() = 0;

  /// Get the static per-gRPC-service metadata provided by codegen
  virtual const ugrpc::impl::StaticServiceMetadata& GetMetadata() const = 0;

  /// Start serving requests. Should be called after the grpcpp server starts.
  virtual void Start() = 0;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
