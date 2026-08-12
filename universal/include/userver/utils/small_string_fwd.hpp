#pragma once

/// @file userver/utils/small_string_fwd.hpp
/// @brief Forward declaration for stack-friendly SmallString.
/// @ingroup userver_universal

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace utils {
// Forward declaration
template <std::size_t N>
class SmallString;
}  // namespace utils

USERVER_NAMESPACE_END
