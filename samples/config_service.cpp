#include <components/minimal_server_component_list.hpp>
#include <components/run.hpp>
#include <fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents
#include <rcu/rcu.hpp>
#include <server/handlers/http_handler_base.hpp>
#include <utils/datetime.hpp>

#include <clients/http/component.hpp>

#include <formats/json.hpp>
#include <formats/json/string_builder.hpp>
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

class ConfigDistributor final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr const char* kName = "handler-config";

  using KeyValues = std::unordered_map<std::string, formats::json::Value>;

  // Component is valid after construction and is able to accept requests
  ConfigDistributor(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase(config, context) {
    auto json = formats::json::FromString(kRuntimeConfig);

    KeyValues new_config;
    for (auto [key, value] : Items(json)) {
      new_config[std::move(key)] = value;
    }

    new_config["USERVER_LOG_REQUEST_HEADERS"] =
        formats::json::ValueBuilder(true).ExtractValue();

    SetNewValues(std::move(new_config));
  }

  const std::string& HandlerName() const override {
    static const std::string kHandlerName = kName;
    return kHandlerName;
  }

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto json = formats::json::FromString(request.RequestBody());

    formats::json::ValueBuilder result;

    const auto config_values_ptr = config_values_.Read();
    result["configs"] = MakeConfigs(config_values_ptr, json);

    const auto updated_at = config_values_ptr->updated_at;
    result["updated_at"] = utils::datetime::Timestring(updated_at);

    return formats::json::ToString(result.ExtractValue());
  }

  void SetNewValues(KeyValues&& key_values) {
    config_values_.Assign(ConfigInfo{
        /*.updated_at=*/utils::datetime::Now(),
        /*.key_values=*/std::move(key_values),
    });
  }

 private:
  struct ConfigInfo {
    std::chrono::system_clock::time_point updated_at;
    std::unordered_map<std::string, formats::json::Value> key_values;
  };

  static formats::json::ValueBuilder MakeConfigs(
      const rcu::ReadablePtr<ConfigInfo>& config_values_ptr,
      const formats::json::Value& json) {
    formats::json::ValueBuilder configs(formats::common::Type::kObject);

    const auto updated_since = json["updated_since"].As<std::string>({});
    if (!updated_since.empty() && utils::datetime::Stringtime(updated_since) >=
                                      config_values_ptr->updated_at) {
      return configs;
    }

    const auto& values = config_values_ptr->key_values;
    if (json["ids"].IsMissing()) {
      // Sending all the configs
      for (const auto& [key, value] : values) {
        configs[key] = value;
      }

      return configs;
    }

    for (const auto& id : json["ids"]) {
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

  rcu::Variable<ConfigInfo> config_values_;
};

}  // namespace samples

const auto kTmpDir = fs::blocking::TempDirectory::Create();
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";

// TODO: purge after TAXICOMMON-3540
const std::string kConfigVariablesPath =
    kTmpDir.GetPath() + "/config_vars.json";
const std::string kConfigVariables =
    "userver-cache-dump-path: " + kTmpDir.GetPath();

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
            bootstrap-path: )~" + kRuntimeConfingPath + R"~(
            fs-cache-path: )~" + kRuntimeConfingPath + R"~(
            fs-task-processor: fs-task-processor
        auth-checker-settings:
        http-server-settings:

        http-client:                      # Component to do HTTP requests
            fs-task-processor: fs-task-processor
            user-agent: 'config-service 1.0'    # Set 'User-Agent' header to 'config-service 1.0'.
        taxi-config-client-updater:
            config-settings: false
            fallback-path: )~" + kRuntimeConfingPath + R"~(
            full-update-interval: 1m
            load-only-my-values: true
            store-enabled: true
            update-interval: 5s
        taxi-configs-client:
            config-url: http://localhost:8083/
            http-retries: 5
            http-timeout: 20s
            service-name: configs-service
            fallback-to-no-proxy: false
        testsuite-support:
        handler-config:
            path: /configs/values
            method: POST              # Only for HTTP POST requests. Other handlers may reuse the same URL but use different method.
            task_processor: main-task-processor
config_vars: )~" + kConfigVariablesPath + R"~(  # TODO: TAXICOMMON-3540
)~";
// clang-format on

int main() {
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, kRuntimeConfig);
  // TODO: purge after TAXICOMMON-3540
  fs::blocking::RewriteFileContents(kConfigVariablesPath, kConfigVariables);

  auto component_list = components::MinimalServerComponentList()  //
                            .Append<components::TaxiConfigClient>()
                            .Append<components::TaxiConfigClientUpdater>()  //

                            .Append<components::HttpClient>()        //
                            .Append<components::TestsuiteSupport>()  //

                            .Append<samples::ConfigDistributor>()  //
      ;
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
