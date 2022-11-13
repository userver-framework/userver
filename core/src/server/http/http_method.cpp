#include <userver/server/http/http_method.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

struct HttpMethodStrigs {
  const std::string kDelete = "DELETE";
  const std::string kGet = "GET";
  const std::string kHead = "HEAD";
  const std::string kPost = "POST";
  const std::string kPut = "PUT";
  const std::string kConnect = "CONNECT";
  const std::string kPatch = "PATCH";
  const std::string kOptions = "OPTIONS";
  const std::string kUnknown = "unknown";
};

// Modern SSO could hold 7 chars without dynamic allocation
const HttpMethodStrigs& GetHttpMethodStrigs() noexcept {
  static const HttpMethodStrigs values{};
  return values;
}

}  // namespace

HttpMethod HttpMethodFromString(std::string_view method_str) {
  const auto& strings = GetHttpMethodStrigs();

  HttpMethod result = HttpMethod::kUnknown;
  if (method_str.size() >= 3) {
    switch (method_str[0]) {
      case 'D':
        if (method_str == strings.kDelete) result = HttpMethod::kDelete;
        break;
      case 'G':
        if (method_str == strings.kGet) result = HttpMethod::kGet;
        break;
      case 'H':
        if (method_str == strings.kHead) result = HttpMethod::kHead;
        break;
      case 'P':
        switch (method_str[1]) {
          case 'A':
            if (method_str == strings.kPatch) result = HttpMethod::kPatch;
            break;
          case 'O':
            if (method_str == strings.kPost) result = HttpMethod::kPost;
            break;
          case 'U':
            if (method_str == strings.kPut) result = HttpMethod::kPut;
            break;
        }
        break;
      case 'C':
        if (method_str == strings.kConnect) result = HttpMethod::kConnect;
        break;
      case 'O':
        if (method_str == strings.kOptions) result = HttpMethod::kOptions;
        break;
    }
  }

  if (result == HttpMethod::kUnknown) {
    throw std::runtime_error("can't parse HttpMethod from string '" +
                             std::string{method_str} + '\'');
  }

  return result;
}

const std::string& ToString(HttpMethod method) noexcept {
  const auto& strings = GetHttpMethodStrigs();

  switch (method) {
    case HttpMethod::kDelete:
      return strings.kDelete;
    case HttpMethod::kGet:
      return strings.kGet;
    case HttpMethod::kHead:
      return strings.kHead;
    case HttpMethod::kPost:
      return strings.kPost;
    case HttpMethod::kPut:
      return strings.kPut;
    case HttpMethod::kConnect:
      return strings.kConnect;
    case HttpMethod::kPatch:
      return strings.kPatch;
    case HttpMethod::kOptions:
      return strings.kOptions;
    case HttpMethod::kUnknown:
      return strings.kUnknown;
  }
  return strings.kUnknown;
}

}  // namespace server::http

USERVER_NAMESPACE_END
