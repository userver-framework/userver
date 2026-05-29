#pragma once

/// @file userver/server/handlers/fallback_handlers.hpp
/// @brief Fallback HTTP handler identifiers and string conversion

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
