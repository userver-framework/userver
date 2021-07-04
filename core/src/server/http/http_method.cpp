#include <userver/server/http/http_method.hpp>

#include <map>

#include <http_parser.h>

namespace server::http {

namespace {

std::map<HttpMethod, std::string> InitHttpMethodNames() {
  std::map<HttpMethod, std::string> names;
  for (auto method : {HttpMethod::kDelete, HttpMethod::kGet, HttpMethod::kHead,
                      HttpMethod::kPost, HttpMethod::kPut, HttpMethod::kPatch,
                      HttpMethod::kConnect, HttpMethod::kOptions}) {
    names[method] = ToString(method);
  }
  return names;
}

std::map<std::string, HttpMethod> InitHttpMethodsMap() {
  static const auto names = InitHttpMethodNames();
  std::map<std::string, HttpMethod> methods_map;
  for (const auto& elem : names) {
    methods_map[elem.second] = elem.first;
  }
  return methods_map;
}

}  // namespace

HttpMethod HttpMethodFromString(const std::string& method_str) {
  static const auto methods_map = InitHttpMethodsMap();
  try {
    return methods_map.at(method_str);
  } catch (std::exception& ex) {
    throw std::runtime_error("can't parse HttpMethod from string '" +
                             method_str + '\'');
  }
}

const std::string& ToString(HttpMethod method) {
  static const std::string kDelete = http_method_str(HTTP_DELETE);
  static const std::string kGet = http_method_str(HTTP_GET);
  static const std::string kHead = http_method_str(HTTP_HEAD);
  static const std::string kPost = http_method_str(HTTP_POST);
  static const std::string kPut = http_method_str(HTTP_PUT);
  static const std::string kConnect = http_method_str(HTTP_CONNECT);
  static const std::string kPatch = http_method_str(HTTP_PATCH);
  static const std::string kOptions = http_method_str(HTTP_OPTIONS);
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
    case HttpMethod::kOptions:
      return kOptions;
    case HttpMethod::kUnknown:
      return kUnknown;
  }
  return kUnknown;
}

}  // namespace server::http
