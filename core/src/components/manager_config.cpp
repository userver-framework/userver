#include <components/manager_config.hpp>

#include <fstream>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <userver/components/static_config_validator.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>
#include <userver/yaml_config/map_to_array.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

formats::yaml::Value ParseYaml(const std::string& doc) { return formats::yaml::FromString(doc); }

formats::yaml::Value ParseYaml(std::istream& is) { return formats::yaml::FromStream(is); }

template <typename T>
ManagerConfig ParseFromAny(
    T&& source,
    const std::string& source_desc,
    const std::optional<std::string>& user_config_vars_path,
    const std::optional<std::string>& user_config_vars_override_path
) {
    constexpr std::string_view kConfigVarsField = "config_vars";
    constexpr std::string_view kManagerConfigField = "components_manager";
    constexpr std::string_view kUserverExperimentsForceEnabledField = "userver_experiments_force_enabled";

    formats::yaml::Value config_yaml;
    try {
        config_yaml = ParseYaml(source);
    } catch (const formats::yaml::Exception& e) {
        throw std::runtime_error("Cannot parse config from '" + source_desc + "': " + e.what());
    }

    std::optional<std::string> config_vars_path = user_config_vars_path;
    if (!config_vars_path) {
        config_vars_path = config_yaml[kConfigVarsField].As<std::optional<std::string>>();
    }
    formats::yaml::Value config_vars;
    if (config_vars_path) {
        config_vars = formats::yaml::blocking::FromFile(*config_vars_path);
    }

    if (user_config_vars_override_path) {
        formats::yaml::ValueBuilder builder = config_vars;

        auto local_config_vars = formats::yaml::blocking::FromFile(*user_config_vars_override_path);
        for (const auto& [name, value] : Items(local_config_vars)) {
            builder[name] = value;
        }
        config_vars = builder.ExtractValue();
    }

    auto config =
        yaml_config::YamlConfig(config_yaml, std::move(config_vars), yaml_config::YamlConfig::Mode::kEnvAndFileAllowed);
    auto result = config[kManagerConfigField].As<ManagerConfig>();
    result.experiments_force_enabled = config[kUserverExperimentsForceEnabledField].As<bool>(false);

    return result;
}

}  // namespace

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
                defaultDescription: 1000
            max_size:
                type: integer
                description: max amount of coroutines to keep preallocated
                defaultDescription: 4000
            stack_size:
                type: integer
                description: size of a single coroutine, bytes
                defaultDescription: 256 * 1024
            local_cache_size:
                type: integer
                description: |
                    Tunes local coroutine cache size per TaskProcessor worker
                    thread. Current coro pool size is computed with
                    an inaccuracy of local_cache_size * total_worker_threads,
                    which may be relevant when comparing against max_size.
                    Lower values of local_cache_size lead to lower performance
                    under heavy contention in the engine, while higher values
                    lead to inaccuracy in coro pool size estimation.
                    local_cache_size=0 disables local cache.
                defaultDescription: 8
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
                        `idle` sets the lowest priority.
                        `low-priority` sets the priority below `normal` but
                        higher than `idle`.
                    defaultDescription: normal
                    enum:
                      - normal
                      - low-priority
                      - idle
                spinning-iterations:
                    type: integer
                    description: |
                        tunes the number of spin-wait iterations in case of
                        an empty task queue before threads go to sleep
                    defaultDescription: 10000
                task-processor-queue:
                    type: string
                    description: |
                        Task queue mode for the task processor.
                        `global-task-queue` default task queue.
                        `work-stealing-task-queue` experimental with
                        potentially better scalability than `global-task-queue`.
                    defaultDescription: global-task-queue
                    enum:
                      - global-task-queue
                      - work-stealing-task-queue
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
    mlock_debug_info:
        type: boolean
        description: whether to mlock(2) process debug info
        defaultDescription: true
    disable_phdr_cache:
        type: boolean
        description: whether to disable caching of phdr_info objects
        defaultDescription: false
    preheat_stacktrace_collector:
        type: boolean
        description: whether to collect a dummy stacktrace at server start up
        defaultDescription: true
    static_config_validation:
        type: object
        description: settings for basic syntax validation in config.yaml
        additionalProperties: false
        properties:
            validate_all_components:
                type: boolean
                description: if true, all components configs are validated
    userver_experiments:
        type: object
        description: userver experiments to enable, `false` by default
        defaultDescription: '{}'
        properties: {}
        additionalProperties:
            type: boolean
            description: whether a specific experiment is enabled
)");
}

ManagerConfig Parse(const yaml_config::YamlConfig& value, formats::parse::To<ManagerConfig>) {
    yaml_config::impl::Validate(value, GetManagerConfigSchema());

    ManagerConfig config;

    config.coro_pool = value["coro_pool"].As<engine::coro::PoolConfig>({});
    config.event_thread_pool = value["event_thread_pool"].As<engine::ev::ThreadPoolConfig>();
    if (config.event_thread_pool.threads < 1) {
        throw std::runtime_error(
            "In static config the components_manager.event_thread_pool.threads "
            "must be greater than 0"
        );
    }
    config.components = yaml_config::ParseMapToArray<components::ComponentConfig>(value["components"]);
    config.task_processors = yaml_config::ParseMapToArray<engine::TaskProcessorConfig>(value["task_processors"]);
    config.default_task_processor = value["default_task_processor"].As<std::string>();

    config.validate_components_configs = value["static_config_validation"].As<ValidationMode>(ValidationMode::kAll);
    config.enabled_experiments = utils::AsContainer<utils::impl::UserverExperimentSet>(
        value["userver_experiments"].As<std::unordered_map<std::string, bool>>({}) |
        boost::adaptors::filtered([](const auto& pair) { return pair.second; }) |
        boost::adaptors::transformed([](const auto& pair) { return pair.first; })
    );
    config.mlock_debug_info = value["mlock_debug_info"].As<bool>(config.mlock_debug_info);
    config.disable_phdr_cache = value["disable_phdr_cache"].As<bool>(config.disable_phdr_cache);
    config.preheat_stacktrace_collector =
        value["preheat_stacktrace_collector"].As<bool>(config.preheat_stacktrace_collector);
    return config;
}

ManagerConfig ManagerConfig::FromString(
    const std::string& str,
    const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path
) {
    return ParseFromAny(str, "<std::string>", config_vars_path, config_vars_override_path);
}

ManagerConfig ManagerConfig::FromFile(
    const std::string& path,
    const std::optional<std::string>& config_vars_path,
    const std::optional<std::string>& config_vars_override_path
) {
    std::ifstream input_stream(path);
    if (!input_stream) {
        throw std::runtime_error("Cannot open config file '" + path + '\'');
    }
    return ParseFromAny(input_stream, path, config_vars_path, config_vars_override_path);
}

}  // namespace components

USERVER_NAMESPACE_END
