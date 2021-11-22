#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep
#include <userver/utils/assert.hpp>

#include <fmt/format.h>
#include <string>
#include <string_view>

/// [Redis service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>

namespace samples::pg {

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const;
  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const;
  std::string DeleteValue(std::string_view key) const;

  storages::redis::ClientPtr redis_client_;
  storages::redis::CommandControl redis_cc_;
};

}  // namespace samples::pg
/// [Redis service sample - component]

namespace samples::pg {

/// [Redis service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient("taxi-tmp")} {}
/// [Redis service sample - component constructor]

/// [Redis service sample - HandleRequestThrow]
std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/) const {
  const auto& key = request.GetArg("key");
  if (key.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'key' query argument"});
  }

  switch (request.GetMethod()) {
    case server::http::HttpMethod::kGet:
      return GetValue(key, request);
    case server::http::HttpMethod::kPost:
      return PostValue(key, request);
    case server::http::HttpMethod::kDelete:
      return DeleteValue(key);
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}
/// [Redis service sample - HandleRequestThrow]

/// [Redis service sample - GetValue]
std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  const auto result = redis_client_->Get(std::string{key}, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }
  return *result;
}
/// [Redis service sample - GetValue]

/// [Redis service sample - PostValue]
std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  const auto result =
      redis_client_->SetIfNotExist(std::string{key}, value, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    return {};
  }

  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  return std::string{value};
}
/// [Redis service sample - PostValue]

/// [Redis service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
  const auto result = redis_client_->Del(std::string{key}, redis_cc_).Get();
  return std::to_string(result);
}
/// [Redis service sample - DeleteValue]

}  // namespace samples::pg
/// [Redis service sample - component]

constexpr std::string_view kDynamicConfig =
    /** [Redis service sample - dynamic config] */ R"~(
{
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE": false,
  "USERVER_RPS_CCONTROL_CUSTOM_STATUS": {},
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
  "REDIS_DEFAULT_COMMAND_CONTROL": {},
  "REDIS_SUBSCRIBER_DEFAULT_COMMAND_CONTROL": {},
  "REDIS_SUBSCRIPTIONS_REBALANCE_MIN_INTERVAL_SECONDS": 30,
  "REDIS_WAIT_CONNECTED": {
    "mode": "master_or_slave",
    "throw_on_fail": false,
    "timeout-ms": 11000
  },
  "REDIS_COMMANDS_BUFFERING_SETTINGS": {
    "buffering_enabled": false,
    "watch_command_timer_interval_us": 0
  }
}
)~"; /** [Redis service sample - dynamic config] */

// clang-format off
constexpr std::string_view kStaticConfigSample = R"~(
# /// [Redis service sample - static config]
# yaml
components_manager:
    components:                       # Configuring components that were registered via component_list
        handler-key-value:
            path: /v1/key-value                  # Registering handler by URL '/v1/key-value'.
            task_processor: main-task-processor  # Run it on CPU bound task processor

        key-value-database:
            groups:
              - config_name: taxi-tmp  # Key to lookup in secdist configuration
                db: taxi-tmp           # Name to refer to the cluster in components::Redis::GetClient()

            subscribe_groups:  # Array of redis clusters to work with in subscribe mode

            thread_pools:
                redis_thread_pool_size: 8
                sentinel_thread_pool_size: 1

        secdist:                                         # Component that stores configuration of hosts and passwords
            config: /etc/redis-service/secdist_cfg.json  # Values are supposed to be stored in this file
            missing-ok: true                             # ... but if the file is missing it is still ok
            environment-secrets-key: SECDIST_CONFIG      # ... values will be loaded from this environment value

        testsuite-support:

        server:
            # ...
# /// [Redis service sample - static config]
            listener:                 # configuring the main listening socket...
                port: 8088            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incomming requests on this task processor.
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stdout'
                    level: debug
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.

        tracer:                           # Component that helps to trace execution times and requests in logs.
            service-name: hello-service   # "You know. You all know exactly who I am. Say my name. " (c)

        taxi-config:                      # Dynamic config storage options, do nothing
            fs-cache-path: ''
        taxi-config-fallbacks:            # Load options from file and push them into the dynamic config storage.
            fallback-path: /etc/redis-service/dynamic_cfg.json
        manager-controller:
        statistics-storage:
        auth-checker-settings:
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
)~";
// clang-format on

// Ad-hoc solution to prepare the environment for tests
const std::string kStaticConfig = [](std::string static_conf) {
  static const auto conf_cache = fs::blocking::TempFile::Create();

  const std::string_view kOrigPath{"/etc/redis-service/dynamic_cfg.json"};
  const auto replacement_pos = static_conf.find(kOrigPath);
  UASSERT(replacement_pos != std::string::npos);
  static_conf.replace(replacement_pos, kOrigPath.size(), conf_cache.GetPath());

  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(conf_cache.GetPath(), kDynamicConfig);

  return static_conf;
}(std::string{kStaticConfigSample});

/// [Redis service sample - main]
int main() {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::pg::KeyValue>()
          .Append<components::Secdist>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>();
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
/// [Redis service sample - main]
