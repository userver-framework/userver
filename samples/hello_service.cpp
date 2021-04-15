#include <components/minimal_server_component_list.hpp>
#include <components/run.hpp>
#include <fs/blocking/read.hpp>            // for fs::blocking::FileExists
#include <fs/blocking/temp_directory.hpp>  // for fs::blocking::TempDirectory
#include <fs/blocking/write.hpp>  // for fs::blocking::RewriteFileContents
#include <server/handlers/http_handler_base.hpp>

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

const auto kTmpDir = fs::blocking::TempDirectory::Create();

// Runtime config values to init the service.
// This is be described in detail in the config_service.cpp sample.
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
  "USERVER_DUMPS": {}
})~";

// clang-format off
const std::string kRuntimeConfingPath =
    kTmpDir.GetPath() + "/runtime_config.json";

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

        taxi-config:                      # Runtime config options. Just loading those from file.
            bootstrap-path: )~" + kRuntimeConfingPath + R"~(
            fs-cache-path: )~" + kRuntimeConfingPath + R"~(
            fs-task-processor: fs-task-processor
        manager-controller:
        statistics-storage:
        auth-checker-settings:
        http-server-settings:

        handler-hello-sample:             # Finally! Our handler.
            path: /hello                  # Registering handler by URL '/hello'.
            task_processor: main-task-processor  # Run it on CPU bound task processor
)~";
// clang-format on

int main() {
  fs::blocking::RewriteFileContents(kRuntimeConfingPath, kRuntimeConfig);

  auto component_list = components::MinimalServerComponentList()  //
                            .Append<samples::hello::Hello>();     //
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
