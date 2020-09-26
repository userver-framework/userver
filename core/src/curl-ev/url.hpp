#pragma once

#include <curl-ev/error_code.hpp>
#include <curl-ev/native.hpp>
#include <curl-ev/wrappers.hpp>

#define IMPLEMENT_CURLU_PART_GET(FUNCTION_NAME, OPTION_NAME)      \
  inline impl::CurlPtr FUNCTION_NAME() {                          \
    std::error_code ec;                                           \
    auto part = FUNCTION_NAME(ec);                                \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                 \
    return part;                                                  \
  }                                                               \
  inline impl::CurlPtr FUNCTION_NAME(std::error_code& ec) {       \
    char* part = nullptr;                                         \
    ec = static_cast<errc::UrlErrorCode>(                         \
        native::curl_url_get(url_.get(), OPTION_NAME, &part, 0)); \
    return impl::CurlPtr{part};                                   \
  }

#define IMPLEMENT_CURLU_PART_SET(FUNCTION_NAME, OPTION_NAME)         \
  inline void FUNCTION_NAME(const char* part) {                      \
    std::error_code ec;                                              \
    FUNCTION_NAME(part, ec);                                         \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                    \
  }                                                                  \
  inline void FUNCTION_NAME(const char* part, std::error_code& ec) { \
    ec = std::error_code{static_cast<errc::UrlErrorCode>(            \
        native::curl_url_set(url_.get(), OPTION_NAME, part, 0))};    \
  }

#define IMPLEMENT_CURLU_PART(PART_NAME, OPTION_NAME)                     \
  IMPLEMENT_CURLU_PART_GET(PP_CONCAT3(Get, PART_NAME, Ptr), OPTION_NAME) \
  IMPLEMENT_CURLU_PART_SET(PP_CONCAT2(Set, PART_NAME), OPTION_NAME)

namespace curl {

class url {
 public:
  url();

  url(const url&);
  url(url&&) = default;
  url& operator=(const url&);
  url& operator=(url&&) = default;

  inline native::CURLU* native_handle() { return url_.get(); }

  // Allows setting URLs relative to the stored one, return normalized version
  IMPLEMENT_CURLU_PART(Url, native::CURLUPART_URL);

  // Always treats passed URL as an absolute one
  void SetAbsoluteUrl(const char* url, std::error_code& ec);
  void SetAbsoluteUrl(const char* url);

  // Backup URL setter with default (https) scheme
  void SetDefaultSchemeUrl(const char* url, std::error_code& ec);

  IMPLEMENT_CURLU_PART(Scheme, native::CURLUPART_SCHEME);
  IMPLEMENT_CURLU_PART(User, native::CURLUPART_USER);
  IMPLEMENT_CURLU_PART(Password, native::CURLUPART_PASSWORD);
  IMPLEMENT_CURLU_PART(Options, native::CURLUPART_OPTIONS);
  IMPLEMENT_CURLU_PART(Host, native::CURLUPART_HOST);
  IMPLEMENT_CURLU_PART(Port, native::CURLUPART_PORT);
  IMPLEMENT_CURLU_PART(Path, native::CURLUPART_PATH);
  IMPLEMENT_CURLU_PART(Query, native::CURLUPART_QUERY);
  IMPLEMENT_CURLU_PART(Fragment, native::CURLUPART_FRAGMENT);

 private:
  impl::UrlPtr url_;
};

}  // namespace curl

#undef IMPLEMENT_CURLU_GET
