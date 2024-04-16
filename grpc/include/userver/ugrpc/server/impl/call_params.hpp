#pragma once

#include <string_view>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/any_storage.hpp>

#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/server/storage_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct CallParams {
  grpc::ServerContext& context;
  const std::string_view call_name;
  ugrpc::impl::RpcStatisticsScope& statistics;
  logging::LoggerRef access_tskv_logger;
  tracing::Span& call_span;
  utils::AnyStorage<StorageContext>& storage_context;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
