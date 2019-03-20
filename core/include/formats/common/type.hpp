#pragma once

/// @file formats/common/type.hpp
/// @brief @copybrief formats::common::Type

namespace formats::common {

/// Common enum of types
enum class Type {
  kNull,     /// Value or ValueBuilder holds Null value
  kArray,    /// Value or ValueBuilder holds an Array
  kObject,   /// Value or ValueBuilder holds a Map
  kMissing,  /// Value or ValueBuilder hold nothing and will throw on an attempt
             /// to get the value
};

}  // namespace formats::common
