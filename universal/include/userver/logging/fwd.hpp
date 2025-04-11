#pragma once

/// @file userver/logging/fwd.hpp
/// @brief Forward declarations for `logging` types

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class LoggerBase;
class TextLogger;

}  // namespace impl

using LoggerRef = impl::LoggerBase&;
using LoggerPtr = std::shared_ptr<impl::LoggerBase>;
using TextLoggerRef = impl::TextLogger&;
using TextLoggerPtr = std::shared_ptr<impl::TextLogger>;

class LogHelper;

class LogExtra;

}  // namespace logging

USERVER_NAMESPACE_END
