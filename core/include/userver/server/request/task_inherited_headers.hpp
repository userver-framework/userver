#pragma once

/// @file userver/server/request/task_inherited_headers.hpp
/// @brief Functions that provide access to incoming headers stored in
/// TaskInheritedVariable.

#include <string>

#include <boost/range/iterator_range.hpp>

#include <userver/utils/impl/transparent_hash.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

using HeadersToPropagate =
    utils::impl::TransparentMap<std::string, std::string>;

/// @brief Get a headers that is handled by the current task hierarchy.
boost::iterator_range<HeadersToPropagate::const_iterator>
GetTaskInheritedHeaders();

/// @brief Set a headers that is handled by the current task hierarchy.
void SetTaskInheritedHeaders(HeadersToPropagate headers);

}  // namespace server::request

USERVER_NAMESPACE_END
