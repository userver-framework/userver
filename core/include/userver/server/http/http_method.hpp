#pragma once

/// @file userver/server/http/http_method.hpp
/// @brief @copybrief server::http::HttpMethod

#include <string>

#include <fmt/core.h>
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief List of HTTP methods
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

/// @brief Convert HTTP method enum value to string
const std::string& ToString(HttpMethod method) noexcept;

/// @brief Convert HTTP method string to enum value
HttpMethod HttpMethodFromString(std::string_view method_str);

}  // namespace server::http

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::server::http::HttpMethod> {
  static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::server::http::HttpMethod method,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::format_to(ctx.out(), "{}",
                          USERVER_NAMESPACE::server::http::ToString(method));
  }
};
