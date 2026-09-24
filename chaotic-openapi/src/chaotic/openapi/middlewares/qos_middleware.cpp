#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>

#include <algorithm>
#include <ranges>
#include <string_view>

#include <fmt/ranges.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

namespace {

constexpr std::string_view kAllowedMethods[] = {"get", "post", "put", "delete", "patch"};

bool IsHttpMethodAllowed(std::string_view method) {
    return std::ranges::find(kAllowedMethods, method) != std::ranges::end(kAllowedMethods);
}

}  // namespace

void impl::WarnOnInvalidQosConfig(
    const dynamic_config::Snapshot& snapshot,
    const dynamic_config::Key<client::CommandControlDict>& key
) {
    const auto& dict = snapshot[key];
    for (const auto& [name, _] : dict) {
        const auto at_pos = name.find('@');
        if (at_pos == std::string::npos) {
            continue;
        }

        const auto method = std::string_view{name}.substr(at_pos + 1);
        if (IsHttpMethodAllowed(method)) {
            continue;
        }

        LOG_ERROR(
            "Found invalid method '{}' for path@method '{}' in dynamic config {}. Allowed methods: {}",
            method,
            name,
            key.GetName(),
            fmt::join(kAllowedMethods, ", ")
        );
    }
}

QosMiddleware::QosMiddleware(dynamic_config::Source source, ConfigKey& key)
    : source_(std::move(source)),
      key_(key)
{}

void QosMiddleware::OnRequest(clients::http::Request& request) {
    const auto& url = request.GetUrl();
    auto path = http::ExtractPathOnly(url);

    auto snapshot = source_.GetSnapshot();
    const auto& dict = snapshot[key_];

    const auto cc = dict.GetOptional(path);
    if (cc) {
        request.timeout(cc->timeout);
        request.retry(cc->attempts);
    }
}

void QosMiddleware::OnResponse(clients::http::Response&) {}

std::string QosMiddleware::GetStaticConfigSchemaStr() {
    return R"(
type: object
description: client QOS middleware configuration
additionalProperties: false
properties: {}
)";
}

}  // namespace chaotic::openapi
USERVER_NAMESPACE_END
