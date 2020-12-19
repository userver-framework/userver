#pragma once

#include <string>

#include <boost/stacktrace/stacktrace_fwd.hpp>

namespace logging {

/// Contains functions that cache stacktrace results
namespace stacktrace_cache {

/// Get cached frame name
std::string to_string(boost::stacktrace::frame frame);

/// Get cached stacktrace
std::string to_string(const boost::stacktrace::stacktrace& st);

};  // namespace stacktrace_cache
}  // namespace logging
