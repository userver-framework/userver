#include <server/request/request_context.hpp>

#include <tracing/tracing.hpp>

namespace server {
namespace request {

namespace {
const std::string kHttpRequestSpanTag = "http_request";
}  // namespace

RequestContext::RequestContext()
    : span_(tracing::Tracer::GetTracer()->CreateSpanWithoutParent(
          kHttpRequestSpanTag)) {}

}  // namespace request

}  // namespace server
