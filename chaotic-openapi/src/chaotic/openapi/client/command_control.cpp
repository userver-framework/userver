#include <userver/chaotic/openapi/client/command_control.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

CommandControl Parse(const formats::json::Value& value, formats::parse::To<CommandControl>) {
    CommandControl cc;
    cc.timeout = value["timeout-ms"].As<std::chrono::milliseconds>();
    cc.attempts = value["attempts"].As<int>();
    return cc;
}

void ApplyConfig(clients::http::Request& request, const CommandControl& cc, const Config& config) {
    if (cc.timeout.count()) {
        request.timeout(cc.timeout.count());
    } else {
        request.timeout(config.timeout.count());
    }

    if (cc.attempts) {
        request.retry(cc.attempts);
    } else {
        request.retry(config.attempts);
    }
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
