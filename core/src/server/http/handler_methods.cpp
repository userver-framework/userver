#include "handler_methods.hpp"

#include <array>

USERVER_NAMESPACE_BEGIN

namespace server::http {
namespace {

std::array<bool, kHandlerMethodsMax + 1> InitHandlerMethodsArray() {
  std::array<bool, kHandlerMethodsMax + 1> is_handler_method{};
  is_handler_method.fill(false);
  for (auto method : kHandlerMethods) {
    is_handler_method[static_cast<size_t>(method)] = true;
  }
  return is_handler_method;
}

}  // namespace

bool IsHandlerMethod(HttpMethod method) {
  static const auto is_handler_method = InitHandlerMethodsArray();
  auto index = static_cast<size_t>(method);
  return index <= kHandlerMethodsMax && is_handler_method[index];
}

}  // namespace server::http

USERVER_NAMESPACE_END
