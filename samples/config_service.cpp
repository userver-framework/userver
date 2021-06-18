#include <components/minimal_server_component_list.hpp>
#include <components/run.hpp>
#include <fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents
#include <rcu/rcu.hpp>
#include <server/handlers/http_handler_json_base.hpp>
#include <utils/datetime.hpp>

#include <clients/http/component.hpp>

#include <formats/json.hpp>
#include <taxi_config/configs/component.hpp>
#include <taxi_config/updater/client/component.hpp>

// Runtime config values to init the service.
constexpr std::string_view kRuntimeConfig = R"~({
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_HTTP_PROXY": "",
  "USERVER_TASK_PROCESSOR_QOS": {
    "default-service": {
      "default-task-processor": {
        "wait_queue_overload": {
          "action": "ignore",
          "length_limit": 5000,
          "time_limit_us": 3000
        }
      }
    }
  },
  "USERVER_CACHES": {},
  "USERVER_LRU_CACHES": {},
  "USERVER_DUMPS": {},
  "HTTP_CLIENT_ENFORCE_TASK_DEADLINE": {
    "cancel-request": false,
    "update-timeout": false
  },
  "HTTP_CLIENT_CONNECTION_POOL_SIZE": 1000,
  "HTTP_CLIENT_CONNECT_THROTTLE": {
    "max-size": 100,
    "token-update-interval-ms": 0
  }
})~";

namespace samples {

/// [Config service sample - component]
struct ConfigDataWithTimestamp {
  std::chrono::system_clock::time_point updated_at;
  std::unordered_map<std::string, formats::json::Value> key_values;
};

class ConfigDistributor final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr const char* kName = "handler-config";

  using KeyValues = std::unordered_map<std::string, formats::json::Value>;

  // Component is valid after construction and is able to accept requests
  ConfigDistributor(const components::ComponentConfig& config,
                    const components::ComponentContext& context);

  const std::string& HandlerName() const override {
    static const std::string kHandlerName = kName;
    return kHandlerName;
  }

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest&, const formats::json::Value& json,
      server::request::RequestContext&) const override;

  void SetNewValues(KeyValues&& key_values) {
    config_values_.Assign(ConfigDataWithTimestamp{
        /*.updated_at=*/utils::datetime::Now(),
        /*.key_values=*/std::move(key_values),
    });
  }

 private:
  rcu::Variable<ConfigDataWithTimestamp> config_values_;
};
/// [Config service sample - component]

ConfigDistributor::ConfigDistributor(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase(config, context) {
  auto json = formats::json::FromString(kRuntimeConfig);

  KeyValues new_config;
  for (auto [key, value] : Items(json)) {
    new_config[std::move(key)] = value;
  }

  new_config["USERVER_LOG_REQUEST_HEADERS"] =
      formats::json::ValueBuilder(true).ExtractValue();

  SetNewValues(std::move(new_config));
}

/// [Config service sample - HandleRequestJsonThrow]
formats::json::ValueBuilder MakeConfigs(
    const rcu::ReadablePtr<ConfigDataWithTimestamp>& config_values_ptr,
    const formats::json::Value& request);

formats::json::Value ConfigDistributor::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value& json,
    server::request::RequestContext&) const {
  formats::json::ValueBuilder result;

  const auto config_values_ptr = config_values_.Read();
  result["configs"] = MakeConfigs(config_values_ptr, json);

  const auto updated_at = config_values_ptr->updated_at;
  result["updated_at"] = utils::datetime::Timestring(updated_at);

  return result.ExtractValue();
}
/// [Config service sample - HandleRequestJsonThrow]

/// [Config service sample - MakeConfigs]
formats::json::ValueBuilder MakeConfigs(
    const rcu::ReadablePtr<ConfigDataWithTimestamp>& config_values_ptr,
    const formats::json::Value& request) {
  formats::json::ValueBuilder configs(formats::common::Type::kObject);

  const auto updated_since = request["updated_since"].As<std::string>({});
  if (!updated_since.empty() && utils::datetime::Stringtime(updated_since) >=
                                    config_values_ptr->updated_at) {
    // Return empty JSON if "updated_since" is sent and no changes since then.
    return configs;
  }

  LOG_DEBUG() << "Sending dynamic config for service "
              << request["service"].As<std::string>("<unknown>");

  const auto& values = config_values_ptr->key_values;
  if (request["ids"].IsMissing()) {
    // Sending all the configs.
    for (const auto& [key, value] : values) {
      configs[key] = value;
    }

    return configs;
  }

  // Sending only the requested configs.
  for (const auto& id : request["ids"]) {
    const auto key = id.As<std::string>();

    const auto it = values.find(key);
    if (it != values.end()) {
      configs[key] = it->second;
    } else {
      LOG_ERROR() << "Failed to find config with name '" << key << "'";
    }
  }

  return configs;
}
/// [Config service sample - MakeConfigs]

}  // namespace samples

const auto kTmpDir = fs::blocking::TempDirectory::Create();
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";

// clang-format off
const std::string kStaticConfig = R"~(
components_manager:
    coro_pool:
        initial_size: 500             # Preallocate 500 coroutines at startup.
        max_size: 1000                # Do not keep more than 1000 preallocated coroutines.

    task_processors:                  # Task processor is an executor for coroutine tasks

        main-task-processor:          # Make a task processor for CPU-bound couroutine tasks.
            worker_threads: 4         # Process tasks in 4 threads.
            thread_name: main-worker  # OS will show the threads of this task processor with 'main-worker' prefix.

        fs-task-processor:            # Make a separate task processor for filesystem bound tasks.
            thread_name: fs-worker
            worker_threads: 4

    default_task_processor: main-task-processor
    components:                       # Configuring components that were registered via component_list

        server:
            listener:                 # configuring the main listening socket...
                port: 8083            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incomming requests on this task processor.
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stderr'
                    level: debug
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.

        tracer:                           # Component that helps to trace execution times and requests in logs.
            service-name: config-service
        manager-controller:
        statistics-storage:
        taxi-config:                      # Runtime config options. Just loading those from file.
            fs-cache-path: )~" + kRuntimeConfingPath + R"~(
            fs-task-processor: fs-task-processor
        auth-checker-settings:
        http-client:                      # Component to do HTTP requests
            fs-task-processor: fs-task-processor
            user-agent: 'config-service 1.0'    # Set 'User-Agent' header to 'config-service 1.0'.
        # /// [Config service sample - config updater static config]
        taxi-configs-client:
            config-url: http://localhost:8083/  # URL of dynamic config service
            http-retries: 5
            http-timeout: 20s
            service-name: configs-service
            fallback-to-no-proxy: false
        taxi-config-client-updater:
            config-settings: false
            fallback-path: )~" + kRuntimeConfingPath + R"~(
            full-update-interval: 1m
            load-only-my-values: true
            store-enabled: true
            update-interval: 5s
        # /// [Config service sample - config updater static config]
        testsuite-support:
        # /// [Config service sample - handler static config]
        handler-config:
            path: /configs/values
            method: POST              # Only for HTTP POST requests. Other handlers may reuse the same URL but use different method.
            task_processor: main-task-processor
        # /// [Config service sample - handler static config]
)~";
// clang-format on

/// [Config service sample - main]
int main() {
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, kRuntimeConfig);

  auto component_list = components::MinimalServerComponentList()  //

                            .Append<components::TaxiConfigClient>()         //
                            .Append<components::TaxiConfigClientUpdater>()  //

                            .Append<components::HttpClient>()        //
                            .Append<components::TestsuiteSupport>()  //

                            .Append<samples::ConfigDistributor>()  //
      ;
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
/// [Config service sample - main]
