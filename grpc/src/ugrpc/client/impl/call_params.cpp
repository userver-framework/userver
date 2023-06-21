#include <userver/ugrpc/client/impl/call_params.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

CallParams DoCreateCallParams(const ClientData& client_data,
                              std::size_t method_id,
                              std::unique_ptr<grpc::ClientContext> context) {
  return CallParams{client_data.GetClientName(),
                    client_data.GetQueue(),
                    client_data.GetConfigSnapshot(),
                    client_data.GetMetadata().method_full_names[method_id],
                    std::move(context),
                    client_data.GetStatistics(method_id),
                    client_data.GetMiddlewares()};
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
