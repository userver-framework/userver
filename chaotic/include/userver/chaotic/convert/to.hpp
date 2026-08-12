#pragma once

/// @file userver/chaotic/convert/to.hpp
/// @brief @copybrief chaotic::convert::to

USERVER_NAMESPACE_BEGIN

namespace chaotic::convert {

/// @brief Helper type for defining `Convert` functions for chaotic.
/// @see @ref convert::ConvertTo
template <typename T>
struct To {};

}  // namespace chaotic::convert

USERVER_NAMESPACE_END
