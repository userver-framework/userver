#pragma once

#include <string>

#include <boost/stacktrace/stacktrace_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// Contains functions that cache stacktrace results
namespace stacktrace_cache {

/// Get cached stacktrace
std::string to_string(const boost::stacktrace::stacktrace& st);

}  // namespace stacktrace_cache
}  // namespace logging

USERVER_NAMESPACE_END
