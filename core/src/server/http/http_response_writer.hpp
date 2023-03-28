#pragma once

#include <userver/engine/io/socket.hpp>
#include <userver/server/http/http_response.hpp>

#include <nghttp2/nghttp2.h>

USERVER_NAMESPACE_BEGIN

namespace server::http {

void WriteHttp1Response(engine::io::Socket& socket,
                        server::http::HttpResponse& response);
void WriteHttp2Response(engine::io::Socket& socket,
                        server::http::HttpResponse& response,
                        concurrent::Variable<impl::SessionPtr>& session_holder,
                        std::optional<Http2UpgradeData> upgrade_data);

}  // namespace server::http

USERVER_NAMESPACE_END
