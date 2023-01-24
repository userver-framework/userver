#pragma once

/// @file userver/logging/null_logger.hpp
/// @brief @copybrief logging::MakeNullLogger()

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class LoggerBase;

}  // namespace impl

using LoggerPtr = std::shared_ptr<impl::LoggerBase>;

/// @brief Creates a logger that drops all incoming messages
/// @see components::Logging
LoggerPtr MakeNullLogger();

}  // namespace logging

USERVER_NAMESPACE_END
