#pragma once

/// @file formats/bson/document.hpp
/// @brief @copybrief formats::bson::Document

#include <formats/bson/types.hpp>
#include <formats/bson/value.hpp>

namespace formats::bson {

/// BSON document
class Document : public Value {
 public:
  /// @brief Constructs an empty document
  /// Equivalent to `MakeDoc()`
  Document();

  /// @brief Unwraps document from Value
  /// @throws TypeMismatchException if value is not a document
  /* implicit */ Document(const Value& value);

  /// @cond
  /// Constructs from a native type, internal use only
  explicit Document(impl::BsonHolder);
  /// @endcond
};

}  // namespace formats::bson
