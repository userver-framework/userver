#include <userver/clients/http/plugins/headers_propagator/plugin.hpp>

#include <userver/server/request/task_inherited_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::headers_propagator {

void HeadersPropagator::PropagateHeaders(
    clients::http::RequestTracingEditor request) const {
  for (const auto& [header, value] :
       server::request::GetTaskInheritedHeaders()) {
    request.SetHeader(header, value);
  }
}

}  // namespace clients::http::plugins::headers_propagator

USERVER_NAMESPACE_END
