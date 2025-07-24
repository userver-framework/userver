#pragma once

/// @file userver/utils/move_only_function.hpp
/// @brief A backport of std::move_only_function from C++23

#include <function_backports/move_only_function.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal userver_containers
///
/// @brief A backport of std::move_only_function from C++23
using function_backports::move_only_function;

}  // namespace utils

USERVER_NAMESPACE_END
