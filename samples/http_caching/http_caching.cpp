#include <userver/utils/assert.hpp>

#include <userver/cache/caching_component_base.hpp>
#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/http/url.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  clients::http::Client& http_client_;
  const std::string translations_url_;
  std::string last_update_remote_;

  formats::json::Value GetAllData() const;
  formats::json::Value GetUpdatedData() const;

  void MergeAndSetData(KeyLangToTranslation&& content,
                       formats::json::Value json,
                       cache::UpdateStatisticsScope& stats_scope);
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
  formats::json::Value json;
  switch (type) {
    case cache::UpdateType::kFull:
      json = GetAllData();
      break;
    case cache::UpdateType::kIncremental:
      json = GetUpdatedData();
      break;
    default:
      UASSERT(false);
  }

  if (json.IsEmpty()) {
    stats_scope.FinishNoChanges();
    return;
  }

  KeyLangToTranslation content;
  if (type == cache::UpdateType::kIncremental) {
    const auto snapshot = Get();  // smart pointer to a shared cache data
    content = *snapshot;          // copying the shared data
  }

  MergeAndSetData(std::move(content), json, stats_scope);
}
/// [HTTP caching sample - update]

/// [HTTP caching sample - GetAllData]
formats::json::Value HttpCachedTranslations::GetAllData() const {
  auto response = http_client_.CreateRequest()
                      .get(translations_url_)  // HTTP GET translations_url_ URL
                      .retry(2)                // retry once in case of error
                      .timeout(std::chrono::milliseconds{500})
                      .perform();  // start performing the request
  response->raise_for_status();
  return formats::json::FromString(response->body_view());
}
/// [HTTP caching sample - GetAllData]

/// [HTTP caching sample - GetUpdatedData]
formats::json::Value HttpCachedTranslations::GetUpdatedData() const {
  const auto url =
      http::MakeUrl(translations_url_, {{"last_update", last_update_remote_}});
  auto response = http_client_.CreateRequest()
                      .get(url)
                      .retry(2)
                      .timeout(std::chrono::milliseconds{500})
                      .perform();
  response->raise_for_status();
  return formats::json::FromString(response->body_view());
}
/// [HTTP caching sample - GetUpdatedData]

/// [HTTP caching sample - MergeAndSetData]
void HttpCachedTranslations::MergeAndSetData(
    KeyLangToTranslation&& content, formats::json::Value json,
    cache::UpdateStatisticsScope& stats_scope) {
  for (const auto& [key, value] : Items(json["content"])) {
    for (const auto& [lang, text] : Items(value)) {
      content.insert_or_assign(KeyLang{key, lang}, text.As<std::string>());
    }
    stats_scope.IncreaseDocumentsReadCount(value.GetSize());
  }

  auto update_time = json["update_time"].As<std::string>();

  const auto size = content.size();
  Set(std::move(content));
  last_update_remote_ = std::move(update_time);
  stats_scope.Finish(size);
}
/// [HTTP caching sample - MergeAndSetData]

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
    const auto& welcome = cache_snapshot->at(KeyLang{"welcome", "ru"});
    return fmt::format("{}, {}! {}", hello, request.GetArg("username"),
                       welcome);
  }

 private:
  samples::http_cache::HttpCachedTranslations& cache_;
};
/// [HTTP caching sample - GreetUser]

yaml_config::Schema HttpCachedTranslations::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<
      components::CachingComponentBase<KeyLangToTranslation>>(R"(
type: object
description: HTTP caching sample component
additionalProperties: false
properties:
    translations-url:
        type: string
        description: some other microservice listens on this URL
)");
}

}  // namespace samples::http_cache

/// [HTTP caching sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::http_cache::HttpCachedTranslations>()
          .Append<samples::http_cache::GreetUser>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<clients::dns::Component>()
          .Append<components::HttpClient>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [HTTP caching sample - main]
