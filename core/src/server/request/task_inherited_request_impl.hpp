#pragma once

#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {
class HttpRequest;
}  // namespace server::http

namespace server::request {

inline engine::TaskInheritedVariable<std::shared_ptr<http::HttpRequest>> kTaskInheritedRequest;

}  // namespace server::request

USERVER_NAMESPACE_END
