#include <server/http/headers_propagator.hpp>

#include <userver/server/request/task_inherited_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

HeadersPropagator::HeadersPropagator(std::vector<std::string>&& headers)
    : headers_(std::move(headers)) {}

void HeadersPropagator::PropagateHeaders(
    clients::http::RequestTracingEditor request) const {
  for (const auto& header : headers_) {
    if (server::request::HasTaskInheritedHeader(header)) {
      request.SetHeader(header,
                        server::request::GetTaskInheritedHeader(header));
    }
  }
}

}  // namespace server::http

USERVER_NAMESPACE_END
