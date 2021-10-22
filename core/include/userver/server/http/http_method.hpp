#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server {
namespace http {

enum class HttpMethod {
  kDelete,
  kGet,
  kHead,
  kPost,
  kPut,
  kPatch,
  kConnect,
  kOptions,
  kUnknown,
};

const std::string& ToString(HttpMethod method);
HttpMethod HttpMethodFromString(const std::string& method_str);

}  // namespace http
}  // namespace server

USERVER_NAMESPACE_END
