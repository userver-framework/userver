#pragma once

USERVER_NAMESPACE_BEGIN

namespace server::http {

class Http2Session;
class HttpResponse;

void WriteHttp2ResponseToSocket(HttpResponse& response, Http2Session& session);

}  // namespace server::http

USERVER_NAMESPACE_END
