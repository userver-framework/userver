#pragma once

/// @file userver/logging/logger.hpp
/// @brief Logger definitions and helpers

#include <memory>
#include <string>

#include <userver/logging/format.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/logging/level.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class TagWriter;
class LoggerBase;
void LogRaw(TextLogger& logger, Level level, std::string_view message);

}  // namespace impl

/// @brief Creates synchronous stderr logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @param format logs output format
/// @param level minimum logs level to write to this logger, a.k.a. logger's log level
/// @see components::Logging
LoggerPtr MakeStderrLogger(const std::string& name, Format format, Level level = Level::kInfo);

/// @brief Creates synchronous stdout logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @param format logs output format
/// @param level minimum logs level to write to this logger, a.k.a. logger's log level
/// @see components::Logging
LoggerPtr MakeStdoutLogger(const std::string& name, Format format, Level level = Level::kInfo);

/// @brief Creates synchronous file logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @param path target log file path
/// @param format logs output format
/// @param level minimum logs level to write to this logger, a.k.a. logger's log level
/// @see components::Logging
LoggerPtr MakeFileLogger(const std::string& name, const std::string& path, Format format, Level level = Level::kInfo);

namespace impl::default_ {

bool DoShouldLog(Level) noexcept;

void PrependCommonTags(TagWriter writer, Level logger_level);

}  // namespace impl::default_

}  // namespace logging

USERVER_NAMESPACE_END
