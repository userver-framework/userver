#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

enum class FallbackHandler {
  kImplicitOptions,
};

constexpr size_t kFallbackHandlerMax =
    static_cast<size_t>(FallbackHandler::kImplicitOptions);

const std::string& ToString(FallbackHandler);

FallbackHandler FallbackHandlerFromString(const std::string& fallback_str);

}  // namespace server::handlers

USERVER_NAMESPACE_END
