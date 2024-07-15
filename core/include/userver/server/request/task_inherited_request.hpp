#pragma once

/// @file userver/server/request/task_inherited_request.hpp
/// @brief Functions that provide access to incoming request stored in
/// TaskInheritedVariable.

#include <memory>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::request {

/// @brief Class for representing incoming request that is handled by the
/// current task hierarchy.
class RequestContainer {
 public:
  virtual std::string_view GetHeader(std::string_view header_name) const = 0;
  virtual bool HasHeader(std::string_view header_name) const = 0;
  virtual ~RequestContainer() = default;
};

/// @brief Set a request that is handled by the current task hierarchy.
void SetTaskInheritedRequest(const RequestContainer& request);

/// @brief Get a header from incoming request that is handled by the
/// current task hierarchy.
/// @return Header value or an empty string, if none such
std::string_view GetTaskInheritedHeader(std::string_view header_name);

/// @brief Checks whether specified header exists in incoming request
/// that is handled by the current task hierarchy.
/// @return `true` if the header exists, `false` otherwise
bool HasTaskInheritedHeader(std::string_view header_name);

}  // namespace server::request

USERVER_NAMESPACE_END
