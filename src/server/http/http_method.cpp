#include <server/http/http_method.hpp>

#include <http_parser.h>

namespace server {
namespace http {

const std::string& ToString(HttpMethod method) {
  static const std::string kDelete = http_method_str(HTTP_DELETE);
  static const std::string kGet = http_method_str(HTTP_GET);
  static const std::string kHead = http_method_str(HTTP_HEAD);
  static const std::string kPost = http_method_str(HTTP_POST);
  static const std::string kPut = http_method_str(HTTP_PUT);
  static const std::string kConnect = http_method_str(HTTP_CONNECT);
  static const std::string kPatch = http_method_str(HTTP_PATCH);
  static const std::string kUnknown = "unknown";

  switch (method) {
    case HttpMethod::kDelete:
      return kDelete;
    case HttpMethod::kGet:
      return kGet;
    case HttpMethod::kHead:
      return kHead;
    case HttpMethod::kPost:
      return kPost;
    case HttpMethod::kPut:
      return kPut;
    case HttpMethod::kConnect:
      return kConnect;
    case HttpMethod::kPatch:
      return kPatch;
    case HttpMethod::kUnknown:
      return kUnknown;
  }
  return kUnknown;
}

}  // namespace http
}  // namespace server
