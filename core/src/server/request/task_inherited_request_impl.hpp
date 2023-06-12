#pragma once

#include <userver/engine/task/inherited_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {
class HttpRequestImpl;
}  // namespace server::http

namespace server::request {

inline engine::TaskInheritedVariable<std::shared_ptr<http::HttpRequestImpl>>
    kTaskInheritedRequest;

}  // namespace server::request

USERVER_NAMESPACE_END
