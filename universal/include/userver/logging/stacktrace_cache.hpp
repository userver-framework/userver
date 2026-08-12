#pragma once

/// @file userver/logging/stacktrace_cache.hpp
/// @brief Cached stringification for Boost stack traces.
/// @ingroup userver_universal

#include <string>

#include <boost/stacktrace/stacktrace_fwd.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Cached stringification for stack traces.
namespace logging::stacktrace_cache {

/// Get cached stacktrace
/// @see GlobalEnableStacktrace
std::string to_string(const boost::stacktrace::stacktrace& st);  // NOLINT(readability-identifier-naming)

/// Enable/disable stacktraces. If disabled, stacktrace_cache::to_string()
/// returns with a const string.
///
/// @note Disabling stacktraces is a hack for the Boost.Stacktrace memory leak
bool GlobalEnableStacktrace(bool enable);

/// RAII-wrapper for `GlobalEnableStacktrace`. Should be used to temporarily
/// enable stacktraces after `GlobalEnableStacktrace(false)`.
class StacktraceGuard {
public:
    explicit StacktraceGuard(bool enabled);

    ~StacktraceGuard();

private:
    const bool old_;
};

}  // namespace logging::stacktrace_cache

USERVER_NAMESPACE_END
