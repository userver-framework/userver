#pragma once

/// @file userver/utils/function_ref.hpp
/// @brief A backport of std::function_ref from C++26
/// @see https://wg21.link/p0792 for reference

#include <function_backports/function_ref.h>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal userver_containers
///
/// @brief A backport of std::function_ref from C++26
/// @see https://wg21.link/p0792 for reference
using function_backports::function_ref;

using function_backports::nontype;
using function_backports::nontype_t;

}  // namespace utils

USERVER_NAMESPACE_END
