#pragma once

#include <algorithm>

#include <server/http/http_method.hpp>

namespace server {
namespace http {

constexpr HttpMethod kHandlerMethods[] = {
    HttpMethod::kGet,    HttpMethod::kPost,  HttpMethod::kPut,
    HttpMethod::kDelete, HttpMethod::kPatch, HttpMethod::kOptions};

constexpr size_t kHandlerMethodsMax = static_cast<size_t>(
    *std::max_element(std::begin(kHandlerMethods), std::end(kHandlerMethods)));

bool IsHandlerMethod(HttpMethod method);

}  // namespace http
}  // namespace server
