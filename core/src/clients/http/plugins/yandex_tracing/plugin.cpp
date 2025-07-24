#include <clients/http/plugins/yandex_tracing/plugin.hpp>

#include <userver/clients/http/response.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <userver/http/common_headers.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::yandex_tracing {

namespace {
constexpr USERVER_NAMESPACE::http::headers::PredefinedHeader kYaTracingHeaders[] = {
    USERVER_NAMESPACE::http::headers::kXRequestId,
    USERVER_NAMESPACE::http::headers::kXBackendServer,
    USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost,
};

const std::string kName = "yandex-tracing";
}  // namespace

Plugin::Plugin() : http::Plugin(kName) {}

void Plugin::HookPerformRequest(PluginRequest&) {}

void Plugin::HookCreateSpan(PluginRequest&, tracing::Span& span) {
    span.AddNonInheritableTag(tracing::kSpanKind, tracing::kSpanKindClient);
}

void Plugin::HookOnCompleted(PluginRequest&, Response& response) {
    const auto& headers = response.headers();
    for (const auto& header : kYaTracingHeaders) {
        const auto header_opt = utils::FindOptional(headers, header);
        if (header_opt) {
            LOG_INFO() << "Client response contains Ya tracing header " << header << "=" << *header_opt;
        }
    }
}

void Plugin::HookOnError(PluginRequest&, std::error_code) {}

bool Plugin::HookOnRetry(PluginRequest&) { return true; }

}  // namespace clients::http::plugins::yandex_tracing

USERVER_NAMESPACE_END
