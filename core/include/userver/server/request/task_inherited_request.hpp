#pragma once

/// @file userver/server/request/task_inherited_request.hpp
/// @brief Functions that provide access to HttpRequest stored in
/// TaskInheritedVariable.

#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::request {

/// @brief Get a header from server::http::HttpRequest that is handled by the
/// current task hierarchy.
/// @return Header value or an empty string, if none such
const std::string& GetTaskInheritedHeader(const std::string& header_name);

/// @brief Checks whether specified header exists in server::http::HttpRequest
/// that is handled by the current task hierarchy.
/// @return `true` if the header exists, `false` otherwise
bool HasTaskInheritedHeader(const std::string& header_name);

}  // namespace server::request

USERVER_NAMESPACE_END
