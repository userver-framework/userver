#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utils/assert.hpp>

#include <userver/cache/caching_component_base.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/components/run.hpp>
#include <userver/http/url.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [HTTP caching sample - datatypes]
namespace samples::http_cache {

struct KeyLang {
  std::string key;
  std::string language;
};

struct KeyLangEq {
  bool operator()(const KeyLang& x, const KeyLang& y) const noexcept;
};

struct KeyLangHash {
  bool operator()(const KeyLang& x) const noexcept;
};

using KeyLangToTranslation =
    std::unordered_map<KeyLang, std::string, KeyLangHash, KeyLangEq>;

}  // namespace samples::http_cache
/// [HTTP caching sample - datatypes]

namespace samples::http_cache {

bool KeyLangEq::operator()(const KeyLang& x, const KeyLang& y) const noexcept {
  return x.key == y.key && x.language == y.language;
}

bool KeyLangHash::operator()(const KeyLang& x) const noexcept {
  std::string data;
  data.append(x.key);
  data.append(x.language);
  return std::hash<std::string>{}(data);
}

}  // namespace samples::http_cache

/// [HTTP caching sample - component]
namespace samples::http_cache {

class HttpCachedTranslations final
    : public components::CachingComponentBase<KeyLangToTranslation> {
 public:
  static constexpr std::string_view kName = "cache-http-translations";

  HttpCachedTranslations(const components::ComponentConfig& config,
                         const components::ComponentContext& context);
  ~HttpCachedTranslations() override;

  void Update(
      cache::UpdateType type,
      [[maybe_unused]] const std::chrono::system_clock::time_point& last_update,
      [[maybe_unused]] const std::chrono::system_clock::time_point& now,
      cache::UpdateStatisticsScope& stats_scope) override;

 private:
  clients::http::Client& http_client_;
  const std::string translations_url_;
  std::string last_update_remote_;

  struct RemoteData {
    std::string update_time;
    KeyLangToTranslation content;
  };

  RemoteData GetAllData() const;
  RemoteData GetUpdatedData() const;
  static void MergeDataInto(RemoteData& data, std::string_view body);
};

}  // namespace samples::http_cache
/// [HTTP caching sample - component]

namespace samples::http_cache {

/// [HTTP caching sample - constructor destructor]
HttpCachedTranslations::HttpCachedTranslations(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : CachingComponentBase(config, context),
      http_client_(
          context.FindComponent<components::HttpClient>().GetHttpClient()),
      translations_url_(config["translations-url"].As<std::string>()) {
  CacheUpdateTrait::StartPeriodicUpdates();
}

HttpCachedTranslations::~HttpCachedTranslations() {
  CacheUpdateTrait::StopPeriodicUpdates();
}
/// [HTTP caching sample - constructor destructor]

/// [HTTP caching sample - update]
void HttpCachedTranslations::Update(
    cache::UpdateType type,
    [[maybe_unused]] const std::chrono::system_clock::time_point& last_update,
    [[maybe_unused]] const std::chrono::system_clock::time_point& now,
    cache::UpdateStatisticsScope& stats_scope) {
  RemoteData result;

  switch (type) {
    case cache::UpdateType::kFull:
      result = GetAllData();
      break;
    case cache::UpdateType::kIncremental:
      result = GetUpdatedData();
      break;
    default:
      UASSERT(false);
  }

  const auto size = result.content.size();
  Set(std::move(result.content));
  last_update_remote_ = std::move(result.update_time);
  stats_scope.Finish(size);
}
/// [HTTP caching sample - update]

/// [HTTP caching sample - GetAllData]
HttpCachedTranslations::RemoteData HttpCachedTranslations::GetAllData() const {
  RemoteData result;
  auto response =
      http_client_.CreateRequest()
          ->post(translations_url_)  // HTTP POST by translations_url_ URL
          ->retry(2)                 // retry once in case of error
          ->timeout(std::chrono::milliseconds{500})
          ->perform();  // start performing the request
  response->raise_for_status();

  MergeDataInto(result, response->body_view());
  return result;
}
/// [HTTP caching sample - GetAllData]

/// [HTTP caching sample - GetUpdatedData]
HttpCachedTranslations::RemoteData HttpCachedTranslations::GetUpdatedData()
    const {
  RemoteData result;

  const auto url =
      http::MakeUrl(translations_url_, {{"last_update", last_update_remote_}});
  auto response = http_client_.CreateRequest()
                      ->post(url)
                      ->retry(2)
                      ->timeout(std::chrono::milliseconds{500})
                      ->perform();
  response->raise_for_status();

  const auto snapshot = Get();  // getting smart pointer to a shared cache data
  result.content = *snapshot;   // copying the shared data
  MergeDataInto(result, response->body_view());
  return result;
}
/// [HTTP caching sample - GetUpdatedData]

/// [HTTP caching sample - MergeDataInto]
void HttpCachedTranslations::MergeDataInto(
    HttpCachedTranslations::RemoteData& data, std::string_view body) {
  formats::json::Value json = formats::json::FromString(body);

  if (json.IsEmpty()) {
    return;
  }

  for (const auto& [key, value] : Items(json["content"])) {
    for (const auto& [lang, text] : Items(value)) {
      data.content.insert_or_assign(KeyLang{key, lang}, text.As<std::string>());
    }
  }

  data.update_time = json["update_time"].As<std::string>();
}
/// [HTTP caching sample - MergeDataInto]

/// [HTTP caching sample - GreetUser]
class GreetUser final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-greet-user";

  GreetUser(const components::ComponentConfig& config,
            const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        cache_(context.FindComponent<HttpCachedTranslations>()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto cache_snapshot = cache_.Get();

    using samples::http_cache::KeyLang;
    const auto& hello = cache_snapshot->at(KeyLang{"hello", "ru"});
    const auto& wellcome = cache_snapshot->at(KeyLang{"wellcome", "ru"});
    return fmt::format("{}, {}! {}", hello, request.GetArg("username"),
                       wellcome);
  }

 private:
  samples::http_cache::HttpCachedTranslations& cache_;
};
/// [HTTP caching sample - GreetUser]

}  // namespace samples::http_cache

constexpr std::string_view kDynamicConfig =
    R"~(
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
)~" /** [HTTP caching sample - dynamic config] */ R"~(
  "HTTP_CLIENT_CONNECTION_POOL_SIZE": 10,
  "HTTP_CLIENT_ENFORCE_TASK_DEADLINE": {
    "cancel-request": false,
    "update-timeout": false
  },
  "HTTP_CLIENT_CONNECT_THROTTLE": {
    "http-limit": 6000,
    "http-per-second": 1500,
    "https-limit": 100,
    "https-per-second": 25,
    "per-host-limit": 3000,
    "per-host-per-second": 500
  }
})~"; /** [HTTP caching sample - dynamic config] */

// clang-format off
constexpr std::string_view kStaticConfigSample = R"~(
# /// [HTTP caching sample - static config cache]
# yaml
components_manager:
    components:                       # Configuring components that were registered via component_list
        cache-http-translations:
            translations-url: http://translations.sample-company.org/v1/translations

            update-types: full-and-incremental
            full-update-interval: 1h
            update-interval: 15m
# /// [HTTP caching sample - static config cache]

# /// [HTTP caching sample - static config deps]
        http-client:
            fs-task-processor: fs-task-processor

        testsuite-support:
        tests-control:
            # Some options from server::handlers::HttpHandlerBase
            path: /tests/control
            task_processor: main-task-processor

        server:
            # ...
# /// [HTTP caching sample - static config deps]
            listener:                 # configuring the main listening socket...
                port: 8089            # ...to listen on this port and...
                task_processor: main-task-processor    # ...process incomming requests on this task processor.
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stdout'
                    level: debug
                    overflow_behavior: discard  # Drop logs if the system is too busy to write them down.

        tracer:                           # Component that helps to trace execution times and requests in logs.
            service-name: http-cache   # "You know. You all know exactly who I am. Say my name. " (c)

        taxi-config:                      # Dynamic config storage options, do nothing
            fs-cache-path: ''
        taxi-config-fallbacks:            # Load options from file and push them into the dynamic config storage.
            fallback-path: /etc/http-cache/dynamic_cfg.json
        manager-controller:
        statistics-storage:
        auth-checker-settings:

        handler-greet-user:
            path: /samples/greet
            task_processor: main-task-processor

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

  {
    const std::string_view kOrigPath{"/etc/http-cache/dynamic_cfg.json"};
    const auto pos = static_conf.find(kOrigPath);
    UASSERT(pos != std::string::npos);
    static_conf.replace(pos, kOrigPath.size(), conf_cache.GetPath());
  }
  {
    const std::string_view kOrigUrl{"translations.sample-company.org"};
    const std::string_view kLocalhost{"localhost:8090"};

    const auto replacement_pos = static_conf.find(kOrigUrl);
    UASSERT(replacement_pos != std::string::npos);
    static_conf.replace(replacement_pos, kOrigUrl.size(), kLocalhost);
  }

  // Use a proper persistent file in production with manually filled values!
  fs::blocking::RewriteFileContents(conf_cache.GetPath(), kDynamicConfig);

  return static_conf;
}(std::string{kStaticConfigSample});

/// [HTTP caching sample - main]
int main() {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::http_cache::HttpCachedTranslations>()
          .Append<samples::http_cache::GreetUser>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<components::HttpClient>();
  components::Run(components::InMemoryConfig{kStaticConfig}, component_list);
}
/// [HTTP caching sample - main]
