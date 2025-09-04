#include <userver/ugrpc/client/retry_config.hpp>

#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

RetryConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<RetryConfig>) {
    RetryConfig retry_config;
    retry_config.attempts = value["attempts"].As<int>(retry_config.attempts);
    return retry_config;
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
