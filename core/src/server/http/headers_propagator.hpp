#pragma once

#include <string>
#include <vector>

#include <userver/clients/http/request_tracing_editor.hpp>
#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HeadersPropagator final {
 public:
  explicit HeadersPropagator(std::vector<std::string>&&);

  void PropagateHeaders(clients::http::RequestTracingEditor request) const;

 private:
  const std::vector<std::string> headers_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
