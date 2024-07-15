#include <userver/server/request/task_inherited_request.hpp>

#include <string>

#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

namespace {
inline engine::TaskInheritedVariable<
    std::reference_wrapper<const RequestContainer>>
    kTaskInheritedRequest;
}  // namespace

void SetTaskInheritedRequest(const RequestContainer& request) {
  kTaskInheritedRequest.Set(request);
}

std::string_view GetTaskInheritedHeader(std::string_view header_name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return {};
  }
  return (*request).get().GetHeader(header_name);
}

bool HasTaskInheritedHeader(std::string_view header_name) {
  const auto* request = kTaskInheritedRequest.GetOptional();
  if (request == nullptr) {
    return false;
  }
  return (*request).get().HasHeader(header_name);
}
}  // namespace server::request

USERVER_NAMESPACE_END
