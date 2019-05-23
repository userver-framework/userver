#pragma once

#include <string>

#include <formats/bson/document.hpp>
#include <formats/bson/types.hpp>
#include <utils/string_view.hpp>

namespace formats::bson {

/// Wraps BSON binary representation
class BsonString;

/// Recovers a BSON document from its binary form
Document FromBinaryString(utils::string_view binary);

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
  utils::string_view GetView() const;

  const uint8_t* Data() const;
  size_t Size() const;

 private:
  impl::BsonHolder impl_;
};

}  // namespace formats::bson
