#include <userver/ugrpc/client/client_settings.hpp>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

std::size_t
GetMethodChannelCount(const DedicatedMethodsConfig& dedicated_methods_config, std::string_view method_name) {
    return utils::FindOrDefault(dedicated_methods_config, std::string{method_name}, std::size_t{0});
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
