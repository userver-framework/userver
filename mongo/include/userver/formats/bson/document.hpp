#pragma once

/// @file userver/formats/bson/document.hpp
/// @brief @copybrief formats::bson::Document

#include <userver/formats/bson/types.hpp>
#include <userver/formats/bson/value.hpp>

USERVER_NAMESPACE_BEGIN

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

  /// Native type access, internal use only
  using Value::GetBson;
  /// @endcond
};

}  // namespace formats::bson

USERVER_NAMESPACE_END
