#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

enum class FallbackHandler {
    kImplicitOptions,
};

inline constexpr size_t kFallbackHandlerMax = static_cast<size_t>(FallbackHandler::kImplicitOptions);

std::string_view ToString(FallbackHandler);

FallbackHandler FallbackHandlerFromString(std::string_view fallback_str);

}  // namespace server::handlers

USERVER_NAMESPACE_END
