#pragma once

/// @file userver/formats/bson/binary.hpp
/// @brief Binary representation helpers

#include <string>
#include <string_view>

#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::bson {

/// Wraps BSON binary representation
class BsonString;

/// Recovers a BSON document from its binary form
Document FromBinaryString(std::string_view binary);

/// Dumps a bson document to a binary string
BsonString ToBinaryString(const formats::bson::Document&);

namespace impl {
class BsonStringImpl;
}  // namespace impl

class BsonString {
 public:
  /// @cond
  explicit BsonString(impl::BsonHolder);
  /// @endcond

  /// Implicitly convertible to string
  /*implicit*/ operator std::string() const { return ToString(); }

  /// Returns a copy of the binary
  std::string ToString() const;

  /// Returns a view of the binary
  std::string_view GetView() const;

  const uint8_t* Data() const;
  size_t Size() const;

 private:
  impl::BsonHolder impl_;
};

}  // namespace formats::bson

USERVER_NAMESPACE_END
