#include "middleware.hpp"

#include <ugrpc/impl/to_string.hpp>
#include <userver/server/request/task_inherited_headers.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::headers_propagator {

Middleware::Middleware(const std::vector<std::string>& headers)
    : headers_(headers) {}

void Middleware::Handle(MiddlewareCallContext& context) const {
  auto& call = context.GetCall();
  const auto& server_context = call.GetContext();
  USERVER_NAMESPACE::server::request::HeadersToPropagate headers_to_propagate;
  for (const auto& header_name : headers_) {
    const auto* header_value = utils::FindOrNullptr(
        server_context.client_metadata(),
        grpc::string_ref{header_name.data(), header_name.size()});
    if (!header_value) {
      continue;
    }
    headers_to_propagate.emplace(header_name,
                                 ugrpc::impl::ToString(*header_value));
  }
  USERVER_NAMESPACE::server::request::SetTaskInheritedHeaders(
      headers_to_propagate);
  context.Next();
}

}  // namespace ugrpc::server::middlewares::headers_propagator

USERVER_NAMESPACE_END
