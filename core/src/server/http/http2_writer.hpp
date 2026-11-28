#pragma once

USERVER_NAMESPACE_BEGIN

namespace server::http {

class Http2Session;
class HttpRequest;

void WriteHttp2ResponseToSocket(HttpRequest& request, Http2Session& session);

}  // namespace server::http

USERVER_NAMESPACE_END
