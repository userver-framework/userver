#pragma once

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Request;
}

namespace chaotic::openapi::client {

struct CommandControl;

/// @brief Applies request-body content-encoding according to cc.encoding
/// Must be called after the request body is set
void EncodeRequest(clients::http::Request& request, const CommandControl& cc);

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
