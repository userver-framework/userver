#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi {

QosMiddleware::QosMiddleware(dynamic_config::Source source, ConfigKey& key) : source_(std::move(source)), key_(key) {}

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
