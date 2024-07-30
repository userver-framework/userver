#pragma once

#include <optional>
#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>

#include <userver/dynamic_config/snapshot.hpp>

#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/impl/client_qos.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/client/qos.hpp>
#include <userver/ugrpc/impl/maybe_owned_string.hpp>
#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct CallParams {
  std::string_view client_name;
  grpc::CompletionQueue& queue;
  dynamic_config::Snapshot config;
  ugrpc::impl::MaybeOwnedString call_name;
  std::unique_ptr<grpc::ClientContext> context;
  ugrpc::impl::MethodStatistics& statistics;
  const Middlewares& mws;
};

CallParams CreateCallParams(const ClientData& client_data,
                            std::size_t method_id,
                            std::unique_ptr<grpc::ClientContext> client_context,
                            const dynamic_config::Key<ClientQos>& client_qos,
                            const Qos& qos);

CallParams CreateGenericCallParams(
    const ClientData& client_data, std::string_view call_name,
    std::unique_ptr<grpc::ClientContext> client_context, const Qos& qos,
    std::optional<std::string_view> metrics_call_name);

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
