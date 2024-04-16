#pragma once

/// @file userver/logging/fwd.hpp
/// @brief Forward declarations for `logging` types

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class LoggerBase;

}  // namespace impl

using LoggerRef = impl::LoggerBase&;
using LoggerPtr = std::shared_ptr<impl::LoggerBase>;

class LogHelper;

class LogExtra;

}  // namespace logging

USERVER_NAMESPACE_END
