#include <userver/ugrpc/client/impl/call_params.hpp>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void CheckValidCallName(std::string_view call_name) {
  UASSERT_MSG(!call_name.empty(), "generic call_name must NOT be empty");
  UASSERT_MSG(call_name[0] != '/',
              utils::StrCat("generic call_name must NOT start with /, given: ",
                            call_name));
  UASSERT_MSG(
      call_name.find('/') != std::string_view::npos,
      utils::StrCat("generic call_name must contain /, given: ", call_name));
}

}  // namespace

CallParams CreateCallParams(const ClientData& client_data,
                            std::size_t method_id,
                            std::unique_ptr<grpc::ClientContext> client_context,
                            const dynamic_config::Key<ClientQos>& client_qos,
                            const Qos& qos) {
  const auto& metadata = client_data.GetMetadata();
  const auto call_name = metadata.method_full_names[method_id];
  const auto method_name =
      call_name.substr(metadata.service_full_name.size() + 1);

  const auto& config = client_data.GetConfigSnapshot();

  // User qos goes first
  ApplyQos(*client_context, qos, client_data.GetTestsuiteControl());

  // If user qos was empty update timeout from config
  ApplyQos(*client_context, config[client_qos][method_name],
           client_data.GetTestsuiteControl());

  return CallParams{
      client_data.GetClientName(),  //
      client_data.GetQueue(),
      client_data.GetConfigSnapshot(),
      {ugrpc::impl::MaybeOwnedString::Ref{}, call_name},
      std::move(client_context),
      client_data.GetStatistics(method_id),
      client_data.GetMiddlewares(),
  };
}

CallParams CreateGenericCallParams(
    const ClientData& client_data, std::string_view call_name,
    std::unique_ptr<grpc::ClientContext> client_context, const Qos& qos,
    std::optional<std::string_view> metrics_call_name) {
  CheckValidCallName(call_name);
  if (metrics_call_name) {
    CheckValidCallName(*metrics_call_name);
  }

  // User qos goes first
  ApplyQos(*client_context, qos, client_data.GetTestsuiteControl());

  return CallParams{
      client_data.GetClientName(),  //
      client_data.GetQueue(),
      client_data.GetConfigSnapshot(),
      ugrpc::impl::MaybeOwnedString{std::string{call_name}},
      std::move(client_context),
      client_data.GetGenericStatistics(metrics_call_name.value_or(call_name)),
      client_data.GetMiddlewares(),
  };
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
