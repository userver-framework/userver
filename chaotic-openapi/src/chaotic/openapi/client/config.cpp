#include <userver/chaotic/openapi/client/config.hpp>

#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

Config ParseConfig(const components::ComponentConfig& config, std::string_view base_url) {
    Config cfg;
    cfg.base_url = config["base-url"].As<std::string>(base_url);
    cfg.attempts = config["attempts"].As<int>(cfg.attempts);
    if (cfg.attempts < 1) throw std::runtime_error("'attempts' must be positive");

    cfg.timeout = std::chrono::milliseconds{config["timeout-ms"].As<int>(cfg.timeout.count())};
    if (cfg.timeout.count() < 1) throw std::runtime_error("'timeout-ms' must be positive");
    return cfg;
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
