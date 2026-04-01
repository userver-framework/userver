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
std::optional<std::string> ReadFile(std::string_view filepath) {
    auto& tp = engine::current_task::GetBlockingTaskProcessor();
    const std::string filepath_str{filepath};
    if (!fs::FileExists(tp, filepath_str)) {
        return std::nullopt;
    }

    return fs::ReadFileContents(tp, filepath_str);
}

bool IsClownductorPrestable() {
    auto content = ReadFile("/etc/clownductor_group");
    if (!content) {
        return false;
    }

    utils::text::Trim(*content);
    return utils::text::EndsWith(*content, "_pre_stable") || utils::text::EndsWith(*content, "_prestable");
}

std::string ReadCircuit() {
    constexpr std::string_view kCircuitPrefix = "UPLATFORM_CIRCUIT=";

    const auto content = ReadFile("/etc/uplatform_environment");
    if (!content) {
        return {};
    }

    for (const auto line : utils::text::SplitIntoStringViewVector(*content, "\n")) {
        const auto trimmed_line = utils::text::TrimView(line);
        if (utils::text::StartsWith(trimmed_line, kCircuitPrefix)) {
            return std::string{trimmed_line.substr(kCircuitPrefix.size())};
        }
    }
    return {};
}
#else
bool IsClownductorPrestable() { return false; }
std::string ReadCircuit() { return {}; }
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
    client_config.circuit = ReadCircuit();
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
