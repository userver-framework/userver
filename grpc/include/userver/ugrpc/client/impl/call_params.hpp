#pragma once

#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/snapshot.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/middleware_fwd.hpp>
#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct CallParams {
  std::string_view client_name;
  grpc::CompletionQueue& queue;
  dynamic_config::Snapshot config;
  std::string_view call_name;
  std::unique_ptr<grpc::ClientContext> context;
  ugrpc::impl::MethodStatistics& statistics;
  const Middlewares& mws;
};

CallParams CreateCallParams(const ClientData&, std::size_t method_id,
                            std::unique_ptr<grpc::ClientContext>);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
