#include <userver/server/request/task_inherited_headers.hpp>

#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
inline engine::TaskInheritedVariable<server::request::HeadersToPropagate>
    kTaskInheritedHeaders;
const server::request::HeadersToPropagate kEmptyHeaders;
}  // namespace

boost::iterator_range<HeadersToPropagate::const_iterator>
GetTaskInheritedHeaders() {
  const auto* headers_ptr = kTaskInheritedHeaders.GetOptional();
  if (headers_ptr == nullptr) {
    return kEmptyHeaders;
  }
  return *headers_ptr;
}

void SetTaskInheritedHeaders(HeadersToPropagate headers) {
  kTaskInheritedHeaders.Set(std::move(headers));
}

}  // namespace server::request

USERVER_NAMESPACE_END
