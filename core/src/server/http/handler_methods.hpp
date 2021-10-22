#pragma once

#include <algorithm>

#include <userver/server/http/http_method.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

inline constexpr HttpMethod kHandlerMethods[] = {
    HttpMethod::kGet,    HttpMethod::kPost,  HttpMethod::kPut,
    HttpMethod::kDelete, HttpMethod::kPatch, HttpMethod::kOptions};

inline constexpr size_t kHandlerMethodsMax = static_cast<size_t>(
    *std::max_element(std::begin(kHandlerMethods), std::end(kHandlerMethods)));

bool IsHandlerMethod(HttpMethod method);

}  // namespace server::http

USERVER_NAMESPACE_END
