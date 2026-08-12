#pragma once

#include <algorithm>

#include <userver/server/http/http_method.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

inline constexpr HttpMethod kHandlerMethods[] = {
    HttpMethod::kGet,
    HttpMethod::kHead,
    HttpMethod::kPost,
    HttpMethod::kPut,
    HttpMethod::kDelete,
    HttpMethod::kPatch,
    HttpMethod::kOptions,
    HttpMethod::kUnknown
};

inline constexpr size_t kHandlerMethodsMax = static_cast<size_t>(*std::ranges::max_element(kHandlerMethods));

bool IsHandlerMethod(HttpMethod method);

}  // namespace server::http

USERVER_NAMESPACE_END
