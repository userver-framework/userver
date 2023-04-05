#include <components/manager_config.hpp>

#include <fstream>

#include <userver/components/static_config_validator.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>
#include <userver/yaml_config/map_to_array.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

formats::yaml::Value ParseYaml(const std::string& doc) {
  return formats::yaml::FromString(doc);
}

formats::yaml::Value ParseYaml(std::istream& is) {
  return formats::yaml::FromStream(is);
}

template <typename T>
ManagerConfig ParseFromAny(
    T&& source, const std::string& source_desc,
    const std::optional<std::string>& user_config_vars_path,
    const std::optional<std::string>& user_config_vars_override_path) {
  static const std::string kConfigVarsField = "config_vars";
  static const std::string kManagerConfigField = "components_manager";
  static const std::string kUserverExperimentsField = "userver_experiments";

  formats::yaml::Value config_yaml;
  try {
    config_yaml = ParseYaml(source);
  } catch (const formats::yaml::Exception& e) {
    throw std::runtime_error("Cannot parse config from '" + source_desc +
                             "': " + e.what());
  }

  std::optional<std::string> config_vars_path = user_config_vars_path;
  if (!config_vars_path) {
    config_vars_path =
        config_yaml[kConfigVarsField].As<std::optional<std::string>>();
  }
  formats::yaml::Value config_vars;
  if (config_vars_path) {
    config_vars = formats::yaml::blocking::FromFile(*config_vars_path);
  }

  if (user_config_vars_override_path) {
    formats::yaml::ValueBuilder builder = config_vars;

    auto local_config_vars =
        formats::yaml::blocking::FromFile(*user_config_vars_override_path);
    for (const auto& [name, value] : Items(local_config_vars)) {
      builder[name] = value;
    }
    config_vars = builder.ExtractValue();
  }

  auto result = yaml_config::YamlConfig(config_yaml[kManagerConfigField],
                                        std::move(config_vars))
                    .As<ManagerConfig>();
  result.enabled_experiments = config_yaml[kUserverExperimentsField]
                                   .As<utils::impl::UserverExperimentSet>({});

  return result;
}

yaml_config::Schema GetManagerConfigSchema() {
  return yaml_config::impl::SchemaFromString(R"(
type: object
description: manager-controller config
additionalProperties: false
properties:
    coro_pool:
        type: object
        description: coroutines pool options
        additionalProperties: false
        properties:
            initial_size:
                type: integer
                description: amount of coroutines to preallocate on startup
            max_size:
                type: integer
                description: max amount of coroutines to keep preallocated
            stack_size:
                type: integer
                description: size of a single coroutine, bytes
                defaultDescription: 256 * 1024
    event_thread_pool:
        type: object
        description: event thread pool options
        additionalProperties: false
        properties:
            threads:
                type: integer
                description: >
                    number of threads to process low level IO system calls
                    (number of ev loops to start in libev)
            defer_events:
                type: boolean
                description: >
                    Whether to defer timer events to a per-thread periodic timer
                    or notify ev-loop right away
    components:
        type: object
        description: 'dictionary of "component name": "options"'
        additionalProperties: true
        properties: {}
    task_processors:
        type: object
        description: dictionary of task processors to create and their options
        additionalProperties:
            type: object
            description: task processor to create and its options
            additionalProperties: false
            properties:
                thread_name:
                    type: string
                    description: set OS thread name to this value
                worker_threads:
                    type: integer
                    description: threads count for the task processor
                guess-cpu-limit:
                    type: boolean
                    description: .
                    defaultDescription: false
                os-scheduling:
                    type: string
                    description: |
                        OS scheduling mode for the task processor threads.
                        `idle` sets the lowest pririty.
                        `low-priority` sets the priority below `normal` but
                        higher than `idle`.
                    defaultDescription: normal
                    enum:
                      - normal
                      - low-priority
                      - idle
                task-trace:
                    type: object
                    description: .
                    additionalProperties: false
                    properties:
                        every:
                            type: integer
                            description: .
                            defaultDescription: 1000
                        max-context-switch-count:
                            type: integer
                            description: .
                            defaultDescription: 0
                        logger:
                            type: string
                            description: .
        properties: {}
    default_task_processor:
        type: string
        description: name of the default task processor to use in components
    static_config_validation:
        type: object
        description: settings for basic syntax validation in config.yaml
        additionalProperties: false
        properties:
            validate_all_components:
                type: boolean
                description: if true, all components configs are validated
    # TODO: remove
    static_config_validator:
        type: object
        description: settings for basic syntax validation in config.yaml
        additionalProperties: false
        properties:
            default_value:
                type: boolean
                description: if true, all components configs are validated
)");
}

}  // namespace

ManagerConfig Parse(const yaml_config::YamlConfig& value,
                    formats::parse::To<ManagerConfig>) {
  yaml_config::impl::Validate(value, GetManagerConfigSchema());

  ManagerConfig config;

  config.coro_pool = value["coro_pool"].As<engine::coro::PoolConfig>();
  config.event_thread_pool =
      value["event_thread_pool"].As<engine::ev::ThreadPoolConfig>();
  if (config.event_thread_pool.threads < 1) {
    throw std::runtime_error(
        "In static config the components_manager.event_thread_pool.threads "
        "must be greater than 0");
  }
  config.components = yaml_config::ParseMapToArray<components::ComponentConfig>(
      value["components"]);
  config.task_processors =
      yaml_config::ParseMapToArray<engine::TaskProcessorConfig>(
          value["task_processors"]);
  config.default_task_processor =
      value["default_task_processor"].As<std::string>();

  config.validate_components_configs =
      value["static_config_validation"].As<ValidationMode>(
          ValidationMode::kAll);
  return config;
}

ManagerConfig ManagerConfig::FromString(
    const std::string& str, const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path) {
  return ParseFromAny(str, "<std::string>", config_vars_path,
                      config_vars_override_path);
}

ManagerConfig ManagerConfig::FromFile(
    const std::string& path, const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path) {
  std::ifstream input_stream(path);
  if (!input_stream) {
    throw std::runtime_error("Cannot open config file '" + path + '\'');
  }
  return ParseFromAny(input_stream, path, config_vars_path,
                      config_vars_override_path);
}

}  // namespace components

USERVER_NAMESPACE_END
