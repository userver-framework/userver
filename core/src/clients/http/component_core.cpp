#include <userver/clients/http/component_core.hpp>

#include <curl/curlver.h>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/server/middlewares/headers_propagator.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/statistics.hpp>
#include <clients/http/testsuite.hpp>
#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/config.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <dynamic_config/variables/HTTP_CLIENT_CONNECTION_POOL_SIZE.hpp>
#include <dynamic_config/variables/HTTP_CLIENT_CONNECT_THROTTLE.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/clients/http/component_core.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

constexpr size_t kDestinationMetricsAutoMaxSizeDefault = 100;

clients::http::ClientSettings GetClientSettings(
    const ComponentConfig& component_config,
    const ComponentContext& context
) {
    clients::http::ClientSettings settings;
    settings = component_config.As<clients::http::ClientSettings>();
    auto& tracing_locator = context.FindComponent<tracing::DefaultTracingManagerLocator>();
    settings.tracing_manager = &tracing_locator.GetTracingManager();
    settings.cancellation_policy =
        component_config["cancellation-policy"]
            .As<clients::http::CancellationPolicy>(clients::http::CancellationPolicy::kCancel);
    return settings;
}

void ValidateCurlVersion() {
    const auto curr_ver = std::make_tuple(LIBCURL_VERSION_MAJOR, LIBCURL_VERSION_MINOR, LIBCURL_VERSION_PATCH);
    if (std::make_tuple(7, 88, 0) <= curr_ver && std::make_tuple(8, 1, 2) >= curr_ver) {
        // See TAXICOMMON-7844
        throw std::runtime_error("Unsupported libcurl " LIBCURL_VERSION
                                 ", versions from 7.88.0 to 8.1.2 are known to "
                                 "crash on HTTP/2 requests");
    }
}

}  // namespace

HttpClientCore::HttpClientCore(const ComponentConfig& component_config, const ComponentContext& context)
    : ComponentBase(component_config, context),
      disable_pool_stats_(component_config["pool-statistics-disable"].As<bool>(false)),
      http_client_(std::make_shared<clients::http::ClientCore>(
          utils::impl::InternalTag{},
          GetClientSettings(component_config, context),
          GetFsTaskProcessor(component_config, context)
      ))
{
    ValidateCurlVersion();

    http_client_
        ->SetDestinationMetricsAutoMaxSize(component_config["destination-metrics-auto-max-size"]
                                               .As<size_t>(kDestinationMetricsAutoMaxSizeDefault));

    http_client_->SetDnsResolver(clients::dns::GetResolverPtr(component_config, context));

    auto user_agent = component_config["user-agent"].As<std::optional<std::string>>();
    if (user_agent) {
        if (!user_agent->empty()) {
            http_client_->ResetUserAgent(std::move(*user_agent));
        } else {
            http_client_->ResetUserAgent({});  // no user agent
        }
    } else {
        // Leaving the default one
    }

    auto testsuite_enabled = component_config["testsuite-enabled"].As<bool>(false);
    if (testsuite_enabled) {
        const auto& timeout = component_config["testsuite-timeout"].As<std::optional<std::chrono::milliseconds>>();
        auto prefixes = component_config["testsuite-allowed-url-prefixes"].As<std::vector<std::string>>({});
        http_client_->SetTestsuiteConfig({prefixes, timeout});

        auto& testsuite = context.FindComponent<components::TestsuiteSupport>();
        testsuite.GetHttpAllowedUrlsExtra().RegisterHttpClient(*http_client_);
    }

    clients::http::impl::Config bootstrap_config;
    http_client_->SetConfig(bootstrap_config);

    auto& config_component = context.FindComponent<components::DynamicConfig>();
    subscriber_scope_ =
        components::DynamicConfig::NoblockSubscriber{config_component}
            .GetEventSource()
            .AddListener(this, kName, &HttpClientCore::OnConfigUpdate);

    const auto thread_name_prefix = component_config["thread-name-prefix"].As<std::string>("");
    auto stats_name = "httpclient" + (thread_name_prefix.empty() ? "" : ("-" + thread_name_prefix));
    utils::statistics::RegisterWriterScope(context, std::move(stats_name), [this](utils::statistics::Writer& writer) {
        return WriteStatistics(writer);
    });
}

HttpClientCore::~HttpClientCore() { subscriber_scope_.Unsubscribe(); }

std::shared_ptr<clients::http::ClientCore> HttpClientCore::GetHttpClientCore(utils::impl::InternalTag) {
    return http_client_;
}

void HttpClientCore::OnConfigUpdate(const dynamic_config::Snapshot& config) {
    http_client_->SetConfig(clients::http::impl::Config{
        config[::dynamic_config::HTTP_CLIENT_CONNECTION_POOL_SIZE],
        clients::http::impl::Parse(config[::dynamic_config::HTTP_CLIENT_CONNECT_THROTTLE]),
    });
}

void HttpClientCore::WriteStatistics(utils::statistics::Writer& writer) {
    if (!disable_pool_stats_) {
        DumpMetric(writer, http_client_->GetPoolStatistics());
    }
    DumpMetric(writer, http_client_->GetDestinationStatistics());
}

yaml_config::Schema HttpClientCore::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/clients/http/component_core.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
