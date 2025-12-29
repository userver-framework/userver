#include <userver/dynamic_config/client/component.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/fs/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/dynamic_config/client/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::string ReadStageName(const std::string& filepath) {
    using formats::json::blocking::FromFile;
    try {
        return FromFile(filepath)["env_name"].As<std::string>();
    } catch (const std::exception& exception) {
        LOG_ERROR()
            << "Error during config service client initialization. "
            << "Got error while reading stage name from file: " << filepath << ", error: " << exception;
        throw;
    }
}

#ifdef ARCADIA_ROOT
bool IsClownductorPrestable() {
    auto filepath = "/etc/clownductor_group";
    auto& tp = engine::current_task::GetBlockingTaskProcessor();

    if (!fs::FileExists(tp, filepath)) {
        return false;
    }
    auto content = fs::ReadFileContents(tp, filepath);
    utils::text::Trim(content);
    return utils::text::EndsWith(content, "_pre_stable") || utils::text::EndsWith(content, "_prestable");
}
#else
bool IsClownductorPrestable() { return false; }
#endif

}  // namespace

DynamicConfigClient::DynamicConfigClient(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase(config, context)
{
    dynamic_config::ClientConfig client_config;
    client_config.service_name = config["service-name"].As<std::string>();
    client_config.get_configs_overrides_for_service = config["get-configs-overrides-for-service"].As<bool>(true);
    client_config.append_path_to_url = config["append-path-to-url"].As<bool>(true);
    client_config.timeout = utils::StringToDuration(config["http-timeout"].As<std::string>());
    client_config.retries = config["http-retries"].As<int>();
    auto stage_filepath = config["configs-stage-filepath"].As<std::optional<std::string>>();
    if (stage_filepath && !stage_filepath->empty()) {
        client_config.stage_name = ReadStageName(*stage_filepath);
    } else {
        auto stage_name = config["configs-stage"].As<std::optional<std::string>>();
        if (stage_name && !stage_name->empty()) {
            client_config.stage_name = *stage_name;
        }
    }
    client_config.is_prestable = IsClownductorPrestable();
    client_config.config_url = config["config-url"].As<std::string>();

    if (!client_config.stage_name.empty() && client_config.get_configs_overrides_for_service) {
        throw std::logic_error("Cannot get overrides for both stage and service yet");
    }

    config_client_ = std::make_unique<dynamic_config::Client>(
        context.FindComponent<HttpClient>(config["http-client"].As<std::string>("dynamic-config-http-client"))
            .GetHttpClient(),
        client_config
    );
}

dynamic_config::Client& DynamicConfigClient::GetClient() const { return *config_client_; }

yaml_config::Schema DynamicConfigClient::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/dynamic_config/client/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
