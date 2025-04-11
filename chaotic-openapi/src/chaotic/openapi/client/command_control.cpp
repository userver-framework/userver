#include <userver/chaotic/openapi/client/command_control.hpp>

#include <userver/clients/http/request.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

void ApplyConfig(clients::http::Request& request, const CommandControl& cc, const Config& config) {
    if (cc) {
        request.timeout(cc.timeout.count());
        request.retry(cc.attempts);
    } else {
        request.timeout(config.timeout.count());
        request.retry(config.attempts);
    }
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
