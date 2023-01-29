#pragma once

#include <stdexcept>
#include <string_view>

#include <userver/http/common_headers.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {
constexpr bool IsLowerCase(std::string_view header) {
  for (const auto c : header) {
    if (c >= 'A' && c <= 'Z') {
      return false;
    }
  }

  return true;
}
}  // namespace impl

struct SpecialHeader final {
  std::string_view name;

  explicit constexpr SpecialHeader(std::string_view name) : name{name} {
    if (!impl::IsLowerCase(name)) {
      throw std::logic_error{"Special header has to be in lowercase"};
    }
  }
};

inline constexpr SpecialHeader kTraceIdHeader{
    USERVER_NAMESPACE::http::headers::kXYaTraceId};

inline constexpr SpecialHeader kSpanIdHeader{
    USERVER_NAMESPACE::http::headers::kXYaSpanId};

inline constexpr SpecialHeader kYaRequestIdHeader{
    USERVER_NAMESPACE::http::headers::kXYaRequestId};

inline constexpr SpecialHeader kRequestIdHeader{
    USERVER_NAMESPACE::http::headers::kXRequestId};

inline constexpr SpecialHeader kBackendServerHeader{
    USERVER_NAMESPACE::http::headers::kXBackendServer};

inline constexpr SpecialHeader kTaxiEnvoyProxyDstVhostHeader{
    USERVER_NAMESPACE::http::headers::kXTaxiEnvoyProxyDstVhost};

inline constexpr SpecialHeader kServerHeader{
    USERVER_NAMESPACE::http::headers::kServer};

inline constexpr SpecialHeader kContentLengthHeader{
    USERVER_NAMESPACE::http::headers::kContentLength};

inline constexpr SpecialHeader kDateHeader{
    USERVER_NAMESPACE::http::headers::kDate};

inline constexpr SpecialHeader kContentTypeHeader{
    USERVER_NAMESPACE::http::headers::kContentType};

inline constexpr SpecialHeader kConnectionHeader{
    USERVER_NAMESPACE::http::headers::kConnection};

}  // namespace server::http

USERVER_NAMESPACE_END
