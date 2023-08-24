#include <curl-ev/url.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

url::url() : url_(native::curl_url()) {}

url::url(const url& other)
    : url_(other.url_ ? native::curl_url_dup(other.url_.get()) : nullptr) {}

url& url::operator=(const url& rhs) {
  if (this == &rhs) return *this;

  *this = url{rhs};
  return *this;
}

void url::SetAbsoluteUrl(const char* url, std::error_code& ec) {
  SetHost(nullptr, ec);
  UASSERT(!ec);
  if (!ec) SetUrl(url, ec);
}

void url::SetAbsoluteUrl(const char* url) {
  std::error_code ec;
  SetAbsoluteUrl(url, ec);
  throw_error(ec, "SetAbsoluteUrl");
}

void url::SetDefaultSchemeUrl(const char* url, std::error_code& ec) {
  SetHost(nullptr, ec);
  UASSERT(!ec);
  if (!ec) {
    ec = std::error_code{static_cast<errc::UrlErrorCode>(native::curl_url_set(
        url_.get(), native::CURLUPART_URL, url, CURLU_DEFAULT_SCHEME))};
  }
}

}  // namespace curl

USERVER_NAMESPACE_END
