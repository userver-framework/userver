#pragma once

#include <string>

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
  kUnknown,
};

const std::string& ToString(HttpMethod method);
HttpMethod HttpMethodFromString(const std::string& method_str);

}  // namespace http
}  // namespace server
