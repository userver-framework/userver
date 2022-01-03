#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utils/assert.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/mongo/component.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [Mongo service sample - component]
namespace samples::mongodb {

class Translations final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-translations";

  Translations(const components::ComponentConfig& config,
               const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        pool_(context.FindComponent<components::Mongo>("mongo-tr").GetPool()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    if (request.GetMethod() == server::http::HttpMethod::kPatch) {
      InsertNew(request);
      return {};
    } else {
      return ReturnDiff(request);
    }
  }

 private:
  void InsertNew(const server::http::HttpRequest& request) const;
  std::string ReturnDiff(const server::http::HttpRequest& request) const;

  storages::mongo::PoolPtr pool_;
};

}  // namespace samples::mongodb
/// [Mongo service sample - component]

namespace samples::mongodb {

/// [Mongo service sample - InsertNew]
void Translations::InsertNew(const server::http::HttpRequest& request) const {
  const auto& key = request.GetArg("key");
  const auto& lang = request.GetArg("lang");
  const auto& value = request.GetArg("value");

  using formats::bson::MakeDoc;
  auto transl = pool_->GetCollection("translations");
  transl.InsertOne(MakeDoc("key", key, "lang", lang, "value", value));
  request.SetResponseStatus(server::http::HttpStatus::kCreated);
}
/// [Mongo service sample - InsertNew]

/// [Mongo service sample - ReturnDiff]
std::string Translations::ReturnDiff(
    const server::http::HttpRequest& request) const {
  auto time_point = std::chrono::system_clock::time_point{};
  if (request.HasArg("last_update")) {
    const auto& update_time = request.GetArg("last_update");
    time_point = utils::datetime::Stringtime(update_time);
  }

  using formats::bson::MakeDoc;
  namespace options = storages::mongo::options;
  auto transl = pool_->GetCollection("translations");
  auto cursor = transl.Find(
      MakeDoc("_id",
              MakeDoc("$gte", formats::bson::Oid::MakeMinimalFor(time_point))),
      options::Sort{std::make_pair("_id", options::Sort::kAscending)});

  if (!cursor) {
    return "{}";
  }

  formats::json::ValueBuilder vb;
  auto content = vb["content"];

  formats::bson::Value last;
  for (const auto& doc : cursor) {
    const auto key = doc["key"].As<std::string>();
    const auto lang = doc["lang"].As<std::string>();
    content[key][lang] = doc["value"].As<std::string>();

    last = doc;
  }

  vb["update_time"] = utils::datetime::Timestring(
      last["_id"].As<formats::bson::Oid>().GetTimePoint());

  return ToString(vb.ExtractValue());
}
/// [Mongo service sample - ReturnDiff]

}  // namespace samples::mongodb

constexpr std::string_view kDynamicConfig =
    /** [Mongo service sample - dynamic config] */ R"~(
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
  "MONGO_DEFAULT_MAX_TIME_MS": 200
}
)~"; /** [Mongo service sample - dynamic config] */

// clang-format off
constexpr std::string_view kStaticConfigSample = R"~(
# /// [Mongo service sample - static config]
# yaml
components_manager:
    components:
        mongo-tr:     # Matches component registration and component retrieval strings
          dbconnection: mongodb://localhost:27217/admin

        handler-translations:
            path: /v1/translations
            task_processor: main-task-processor

        server:
            # ...
# /// [Mongo service sample - static config]
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

  const std::string_view kOrigPath{"/etc/translations-sample/dynamic_cfg.json"};
  const auto pos = static_conf.find(kOrigPath);
  UASSERT(pos != std::string::npos);
  static_conf.replace(pos, kOrigPath.size(), conf_cache.GetPath());

  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(conf_cache.GetPath(), kDynamicConfig);

  return static_conf;
}(std::string{kStaticConfigSample});

/// [Mongo service sample - main]
int main() {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<components::Mongo>("mongo-tr")
                                  .Append<samples::mongodb::Translations>();
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
/// [Mongo service sample - main]
