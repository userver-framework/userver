#include <components/minimal_server_component_list.hpp>
#include <components/run.hpp>
#include <fs/blocking/read.hpp>   // for fs::blocking::FileExists
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

namespace cache {
void CacheConfigInit() {}  // TODO: purge!
}  // namespace cache

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
  }
})~";

const std::string kConfingCachePath = "/tmp/userver_config_bootstrap.json";

const std::string kStaticConfig = R"~(
components_manager:
    coro_pool:
        initial_size: 500
        max_size: 1000
    task_processors:
        main-task-processor:
            worker_threads: 4
            thread_name: main-worker
        fs-task-processor:
            thread_name: fs-worker
            worker_threads: 4
    default_task_processor: main-task-processor
    components:
        server:
            listener:
                port: 8080
                task_processor: main-task-processor
        logging:
            fs-task-processor-name: fs-task-processor
            loggers:
                default:
                    file_path: '@stderr'
                    level: debug
                    overflow_behavior: discard
        tracer:
            service-name: hello-service
            tracer: native
        manager-controller:
        statistics-storage:
        taxi-config:
            bootstrap-path: /tmp/userver_config_bootstrap.json
            fs-cache-path: /tmp/userver_config_bootstrap.json
            fs-task-processor-name: fs-task-processor
        handler-hello-sample:
            path: /hello
            task_processor: main-task-processor
        auth-checker-settings:
        http-server-settings:
)~";

int main() {
  if (!fs::blocking::FileExists(kConfingCachePath)) {
    fs::blocking::RewriteFileContents(kConfingCachePath, kRuntimeConfig);
  }

  auto component_list = components::MinimalServerComponentList()  //
                            .Append<samples::hello::Hello>();     //
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
