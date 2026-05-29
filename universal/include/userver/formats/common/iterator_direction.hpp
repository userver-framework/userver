#pragma once

/// @file userver/formats/common/iterator_direction.hpp
/// @brief @copybrief formats::common::IteratorDirection
/// @ingroup userver_universal

USERVER_NAMESPACE_BEGIN

namespace formats::common {

/// @brief Helper to distinguish forward and reverse iterators for Value
enum class IteratorDirection {
    kForward = 1,
    kReverse = -1,
};

}  // namespace formats::common

USERVER_NAMESPACE_END
