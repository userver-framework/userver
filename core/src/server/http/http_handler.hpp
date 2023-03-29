#pragma once

#include <userver/engine/io/socket.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HTTPHandler {
 public:
  virtual bool Parse(const char* data, size_t size) = 0;
  virtual void SendResponse(engine::io::Socket& socket,
                            request::ResponseBase& response) const = 0;
};

}  // namespace server::http

USERVER_NAMESPACE_END
