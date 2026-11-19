#include <userver/ugrpc/client/client_factory_settings.hpp>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

std::shared_ptr<grpc::ChannelCredentials> GetClientCredentials(
    const ClientFactorySettings& client_factory_settings,
    const std::string& client_name
) {
    if (const auto* credentials = utils::FindOrNullptr(client_factory_settings.client_credentials, client_name)) {
        return *credentials;
    }
    return client_factory_settings.credentials
               ? client_factory_settings.credentials
               : grpc::InsecureChannelCredentials();
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
