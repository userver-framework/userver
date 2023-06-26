#pragma once

#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/snapshot.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
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

CallParams DoCreateCallParams(const ClientData&, std::size_t method_id,
                              std::unique_ptr<grpc::ClientContext>);

template <typename ClientQosConfig>
CallParams CreateCallParams(const ClientData& client_data,
                            std::size_t method_id,
                            std::unique_ptr<grpc::ClientContext> client_context,
                            const ClientQosConfig& client_qos) {
  const auto& metadata = client_data.GetMetadata();
  const auto& full_name = metadata.method_full_names[method_id];
  const auto& method_name =
      full_name.substr(metadata.service_full_name.size() + 1);

  const auto& config = client_data.GetConfigSnapshot();
  ApplyQos(*client_context, config[client_qos][method_name],
           client_data.GetTestsuiteControl());
  return DoCreateCallParams(client_data, method_id, std::move(client_context));
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
