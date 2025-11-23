#pragma once

/// @file userver/logging/null_logger.hpp
/// @brief @copybrief logging::MakeNullLogger()

#include <userver/logging/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// @brief Returns a logger that drops all incoming messages
/// @see components::Logging
TextLoggerRef GetNullLogger() noexcept;

/// @brief Creates a logger that drops all incoming messages.
///
/// Use GetNullLogger() is you need a reference to logger.
///
/// @see components::Logging
TextLoggerPtr MakeNullLogger();

namespace impl {

// Creates a logger that drops all incoming messages, but
// * reports log level INFO (customizable) to force forming those messages;
// * uses Formats::kRaw to produce reasonable CPU usage for formatting logs.
TextLoggerPtr MakeNoopLoggerForTests();

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
