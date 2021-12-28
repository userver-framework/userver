#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utils/assert.hpp>

// TODO: make it a mongo sample!

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace samples::mongodb {

constexpr std::string_view kFullDataUpdateTime = "2021-11-01T12:00:00z";
constexpr std::string_view kIncrementalUpdateTime = "2021-12-01T12:00:00z";

class BulkTranslations final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-bulk-translations";

  BulkTranslations(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
      : HttpHandlerBase(config, context) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    formats::json::ValueBuilder vb;
    vb["content"]["hello"]["ru"] = "Привет";
    vb["content"]["wellcome"]["ru"] = "Добро пожаловать";
    vb["content"]["hello"]["en"] = "Hello";
    vb["content"]["wellcome"]["en"] = "Wellcome";

    vb["update_time"] = kFullDataUpdateTime;
    return ToString(vb.ExtractValue());
  }
};

class IncrementalTranslations final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-incremental-translations";
  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto& update_time = request.GetArg("last_update");
    if (update_time != kFullDataUpdateTime) {
      // return "{}";
    }

    formats::json::ValueBuilder vb;
    vb["content"]["hello"]["ru"] = "Приветище";
    vb["update_time"] = kIncrementalUpdateTime;
    return ToString(vb.ExtractValue());
  }
};

}  // namespace samples::mongodb

constexpr std::string_view kDynamicConfig =
    /** [Hello service sample - dynamic config] */ R"~(
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
                port: 8090            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incomming requests on this task processor.
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stdout'
                    level: debug
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.

        tracer:                           # Component that helps to trace execution times and requests in logs.
            service-name: translations-sample   # "You know. You all know exactly who I am. Say my name. " (c)

        taxi-config:                      # Dynamic config storage options, do nothing
            fs-cache-path: ''
        taxi-config-fallbacks:            # Load options from file and push them into the dynamic config storage.
            fallback-path: /etc/translations-sample/dynamic_cfg.json
        manager-controller:
        statistics-storage:
        auth-checker-settings:

        handler-bulk-translations:
            path: /translations/v1/bulk
            task_processor: main-task-processor
        handler-incremental-translations:
            path: /translations/v1/bulk-incremental
            task_processor: main-task-processor
# /// [Hello service sample - static config]
)~";
// clang-format on

// Ad-hoc solution to prepare the environment for tests
const std::string kStaticConfig = [](std::string static_conf) {
  static const auto conf_cache = fs::blocking::TempFile::Create();

  const std::string_view kOrigPath{"/etc/translations-sample/dynamic_cfg.json"};
  const auto pos = static_conf.find(kOrigPath);
  UASSERT(pos != std::string::npos);
  static_conf.replace(pos, kOrigPath.size(), conf_cache.GetPath());

  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(conf_cache.GetPath(), kDynamicConfig);

  return static_conf;
}(std::string{kStaticConfigSample});

int main() {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::mongodb::BulkTranslations>()
          .Append<samples::mongodb::IncrementalTranslations>();
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
