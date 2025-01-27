#include <userver/ugrpc/client/client_factory_settings.hpp>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

std::shared_ptr<grpc::ChannelCredentials>
GetClientCredentials(const ClientFactorySettings& client_factory_settings, const std::string& client_name) {
    return utils::FindOrDefault(
        client_factory_settings.client_credentials, client_name, client_factory_settings.credentials
    );
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
