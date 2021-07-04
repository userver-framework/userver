#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utils/assert.hpp>

/// [Hello service sample - component]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

namespace samples::hello {

class Hello final : public server::handlers::HttpHandlerBase {
 public:
  // `kName` must match component name in config.yaml
  static constexpr const char* kName = "handler-hello-sample";

  // Component is valid after construction and is able to accept requests
  Hello(const components::ComponentConfig& config,
        const components::ComponentContext& context)
      : server::handlers::HttpHandlerBase(config, context) {}

  const std::string& HandlerName() const override {
    static const std::string kHandlerName = kName;
    return kHandlerName;
  }

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    return "Hello world!\n";
  }
};

}  // namespace samples::hello
/// [Hello service sample - component]

constexpr std::string_view kDynamicConfig =
    /** [Hello service sample - dynamic config] */ R"~(
{
  "USERVER_TASK_PROCESSOR_PROFILER_DEBUG": {},
  "USERVER_LOG_REQUEST": true,
  "USERVER_LOG_REQUEST_HEADERS": false,
  "USERVER_CHECK_AUTH_IN_HANDLERS": false,
  "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE": false,
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
  "USERVER_DUMPS": {}
}
)~"; /** [Hello service sample - dynamic config] */

// clang-format off
constexpr std::string_view kStaticConfigSample = R"~(
# /// [Hello service sample - static config]
# yaml
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
                port: 8080            # ...to listen on this port and...
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

        taxi-config:                      # dynamic config options. Just loading those from file.
            fs-cache-path: /var/cache/hello-service/dynamic_cfg.json
            fs-task-processor: fs-task-processor
        manager-controller:
        statistics-storage:
        auth-checker-settings:

        handler-hello-sample:             # Finally! Our handler.
            path: /hello                  # Registering handler by URL '/hello'.
            task_processor: main-task-processor  # Run it on CPU bound task processor
# /// [Hello service sample - static config]
)~";
// clang-format on

// Ad-hoc solution to prepare the environment for tests
const std::string kStaticConfig = [](std::string static_conf) {
  static const auto conf_cache = fs::blocking::TempFile::Create();

  const std::string_view kOrigPath{"/var/cache/hello-service/dynamic_cfg.json"};
  const auto replacement_pos = static_conf.find(kOrigPath);
  UASSERT(replacement_pos != std::string::npos);
  static_conf.replace(replacement_pos, kOrigPath.size(), conf_cache.GetPath());

  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(conf_cache.GetPath(), kDynamicConfig);

  return static_conf;
}(std::string{kStaticConfigSample});

/// [Hello service sample - main]
int main() {
  const auto component_list =
      components::MinimalServerComponentList().Append<samples::hello::Hello>();
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
/// [Hello service sample - main]
