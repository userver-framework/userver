#pragma once

/// @file userver/logging/null_logger.hpp
/// @brief @copybrief logging::MakeNullLogger()

#include <userver/logging/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// @brief Returns a logger that drops all incoming messages
/// @see components::Logging
LoggerRef GetNullLogger() noexcept;

/// @brief Creates a logger that drops all incoming messages.
///
/// Use GetNullLogger() is you need a reference to logger.
///
/// @see components::Logging
LoggerPtr MakeNullLogger();

}  // namespace logging

USERVER_NAMESPACE_END
