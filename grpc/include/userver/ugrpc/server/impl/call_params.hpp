#pragma once

#include <string_view>

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/tracing/span.hpp>

#include <userver/ugrpc/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

struct CallParams {
  grpc::ServerContext& context;
  const std::string_view call_name;
  ugrpc::impl::RpcStatisticsScope& statistics;
  logging::LoggerCRef access_tskv_logger;
  tracing::Span& call_span;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
