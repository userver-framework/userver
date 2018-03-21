#include "http_request_constructor.hpp"

#include <algorithm>
#include <cassert>
#include <sstream>

#include <logging/logger.hpp>

namespace server {
namespace http {

namespace {

const std::string kCookieHeader = "Cookie";

inline void Strip(const char*& begin, const char*& end) {
  while (begin < end && isspace(*begin)) ++begin;
  while (begin < end && isspace(end[-1])) --end;
}

}  // namespace

HttpRequestConstructor::HttpRequestConstructor()
    : request_(std::make_unique<HttpRequestImpl>()) {}

HttpRequestConstructor::~HttpRequestConstructor() {}

void HttpRequestConstructor::SetMethod(http_method method) {
  request_->method_ = method;
}

void HttpRequestConstructor::SetHttpMajor(unsigned short http_major) {
  request_->http_major_ = http_major;
}

void HttpRequestConstructor::SetHttpMinor(unsigned short http_minor) {
  request_->http_minor_ = http_minor;
}

void HttpRequestConstructor::SetParseArgsFromBody(bool parse_args_from_body) {
  parse_args_from_body_ = parse_args_from_body;
}

void HttpRequestConstructor::AppendUrl(const char* data, size_t size) {
  request_->url_.append(data, size);
}

void HttpRequestConstructor::AppendHeaderField(const char* data, size_t size) {
  if (header_value_flag_) {
    AddHeader();
    header_value_flag_ = false;
  }
  header_field_flag_ = true;
  header_field_.append(data, size);
}

void HttpRequestConstructor::AppendHeaderValue(const char* data, size_t size) {
  assert(header_field_flag_);
  header_value_flag_ = true;
  header_value_.append(data, size);
}

void HttpRequestConstructor::AppendBody(const char* data, size_t size) {
  request_->request_body_.append(data, size);
}

namespace {

template <typename Array, typename PrintElem>
std::ostringstream& PrintArray(std::ostringstream& os, const std::string& name,
                               const Array& array, PrintElem print_elem) {
  bool first = true;
  os << name << ":[";
  for (const auto& elem : array) {
    if (first)
      first = false;
    else
      os << ", ";
    print_elem(elem);
  }
  os << ']';
  return os;
}

}  // namespace

std::unique_ptr<request::RequestBase> HttpRequestConstructor::Finalize() {
  http_parser_url url;

  LOG_TRACE() << "method=" << request_->GetMethodStr();
  LOG_TRACE() << "parse path from '" << request_->url_ << "'";
  if (http_parser_parse_url(request_->url_.data(), request_->url_.size(),
                            request_->method_ == HTTP_CONNECT, &url)) {
    LOG_DEBUG() << "can't parse url";
    return nullptr;
  }

  if (url.field_set & (1 << http_parser_url_fields::UF_PATH)) {
    const auto& str_info = url.field_data[http_parser_url_fields::UF_PATH];
    request_->request_path_ = request_->url_.substr(str_info.off, str_info.len);
    LOG_TRACE() << "path='" << request_->request_path_ << "'";
  } else {
    LOG_DEBUG() << "can't parse path";
    return nullptr;
  }

  try {
    ParseArgs(url);
    if (parse_args_from_body_)
      ParseArgs(request_->request_body_.data(), request_->request_body_.size());
  } catch (const std::exception& ex) {
    LOG_DEBUG() << "can't parse args: " << ex.what();
    return nullptr;
  }

  assert(std::all_of(request_->request_args_.begin(),
                     request_->request_args_.end(),
                     [](const auto& arg) { return !arg.second.empty(); }));

  auto print_request_args = [this]() {
    std::ostringstream os;
    PrintArray(
        os, "request_args", request_->request_args_,
        [&os](const std::pair<std::string, std::vector<std::string>>& arg) {
          os << "{\"arg_name\":\"" << arg.first << "\", ";
          PrintArray(
              os, "\"arg_values\"", arg.second,
              [&os](const std::string& value) { os << "\"" << value << "\""; });
        });
    return os.str();
  };
  LOG_TRACE() << print_request_args();

  auto print_headers = [this]() {
    std::ostringstream os;
    PrintArray(os, "headers", request_->headers_,
               [&os](const std::pair<std::string, std::string>& header) {
                 os << "{\"header_name\":\"" << header.first
                    << "\", \"header_value\":\"" << header.second << "\"}";
               });
    return os.str();
  };
  LOG_TRACE() << print_headers();

  try {
    ParseCookies();
  } catch (const std::exception& ex) {
    LOG_DEBUG() << "can't parse cookies: " << ex.what();
    return nullptr;
  }
  auto print_cookies = [this]() {
    std::ostringstream os;
    PrintArray(os, "cookies", request_->cookies_,
               [&os](const std::pair<std::string, std::string>& cookie) {
                 os << "{\"cookie_name\":\"" << cookie.first
                    << "\", \"cookie_value\":\"" << cookie.second << "\"}";
               });
    return os.str();
  };
  LOG_TRACE() << print_cookies();

  return std::move(request_);
}

std::string HttpRequestConstructor::UrlDecode(const char* data,
                                              const char* data_end) {
  std::string res;
  for (const char* ptr = data; ptr < data_end; ++ptr) {
    if (*ptr == '%') {
      if (ptr + 2 < data_end && isxdigit(ptr[1]) && isxdigit(ptr[2])) {
        unsigned int num;
        sscanf(ptr + 1, "%2x", &num);
        res += static_cast<char>(num);
        ptr += 2;
      } else {
        throw std::runtime_error("invalid percent-encoding sequence");
      }
    } else {
      res += *ptr;
    }
  }
  return res;
}

void HttpRequestConstructor::ParseArgs(const http_parser_url& url) {
  if (url.field_set & (1 << http_parser_url_fields::UF_QUERY)) {
    const auto& str_info = url.field_data[http_parser_url_fields::UF_QUERY];
    ParseArgs(request_->url_.data() + str_info.off, str_info.len);
    LOG_TRACE() << "query="
                << request_->url_.substr(str_info.off, str_info.len);
  }
}

void HttpRequestConstructor::ParseArgs(const char* data, size_t size) {
  const char* end = data + size;
  const char* key_begin = data;
  const char* key_end = data;
  bool parse_key = true;
  for (const char* ptr = data; ptr <= end; ++ptr) {
    if (ptr == end || *ptr == '&') {
      if (!parse_key) {
        const char* value_begin = key_end + 1;
        const char* value_end = ptr;
        if (key_begin < key_end && value_begin < value_end) {
          std::vector<std::string>& arg_values =
              request_->request_args_[UrlDecode(key_begin, key_end)];
          arg_values.emplace_back(UrlDecode(value_begin, value_end));
        }
      }
      parse_key = true;
      key_begin = ptr + 1;
      continue;
    }
    if (*ptr == '=' && parse_key) {
      parse_key = false;
      key_end = ptr;
      continue;
    }
  }
}

void HttpRequestConstructor::AddHeader() {
  assert(header_field_flag_);
  request_->headers_.emplace(std::move(header_field_),
                             std::move(header_value_));
  header_field_.clear();
  header_value_.clear();
}

void HttpRequestConstructor::ParseCookies() {
  const std::string& cookie = request_->GetHeader(kCookieHeader);

  const char* data = cookie.data();
  size_t size = cookie.size();
  const char* end = data + size;
  const char* key_begin = data;
  const char* key_end = data;
  bool parse_key = true;
  for (const char* ptr = data; ptr <= end; ++ptr) {
    if (ptr == end || *ptr == ';') {
      const char* value_begin;
      const char* value_end;
      if (parse_key) {
        key_end = ptr;
        value_begin = value_end = ptr;
      } else {
        value_begin = key_end + 1;
        value_end = ptr;
        Strip(value_begin, value_end);
        if (value_begin + 2 <= value_end && *value_begin == '"' &&
            value_end[-1] == '"') {
          ++value_begin;
          --value_end;
        }
      }
      Strip(key_begin, key_end);
      if (key_begin < key_end)
        request_->cookies_.emplace(UrlDecode(key_begin, key_end),
                                   UrlDecode(value_begin, value_end));
      parse_key = true;
      key_begin = ptr + 1;
      continue;
    }
    if (*ptr == '=' && parse_key) {
      parse_key = false;
      key_end = ptr;
      continue;
    }
  }
}

}  // namespace http
}  // namespace server
