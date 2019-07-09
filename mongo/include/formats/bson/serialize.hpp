#pragma once

/// @file formats/bson/serialize.hpp
/// @brief Textual serialization helpers

#include <iosfwd>
#include <string>

#include <formats/bson/document.hpp>
#include <utils/fast_pimpl.hpp>
#include <utils/string_view.hpp>

namespace formats::bson {

/// Wraps To*JsonString results to avoid unneeded copying
class JsonString;

/// Applies heuristics to convert JSON string to BSON document.
/// As JSON have rather lax typing, some heuristics are applied to guess correct
/// BSON types for values. It is strongly recommended to write your own
/// conversion routines matching your schemas.
/// @warning Stability of heuristics is not guaranteed, this is provided as-is.
Document FromJsonString(utils::string_view json);

/// Converts BSON to a canonical MongoDB Extended JSON format.
/// @see
/// https://github.com/mongodb/specifications/blob/master/source/extended-json.rst
JsonString ToCanonicalJsonString(const formats::bson::Document&);

/// Converts BSON to a relaxed MongoDB Extended JSON format.
/// Notable differences from canonical format are:
///  * numbers are not being wrapped in `$number*` objects;
///  * dates have string representation.
/// @see
/// https://github.com/mongodb/specifications/blob/master/source/extended-json.rst
JsonString ToRelaxedJsonString(const formats::bson::Document&);

/// Converts BSON to a legacy libbson's JSON format.
/// Notable differences from other formats:
///  * all numbers are not wrapped;
///  * non-standard tokens for special floating point values;
///  * dates are milliseconds since epoch;
///  * different format for binaries.
JsonString ToLegacyJsonString(const formats::bson::Document&);

namespace impl {
class JsonStringImpl;
}  // namespace impl

class JsonString {
 public:
  /// @cond
  explicit JsonString(impl::JsonStringImpl&&);
  /// @endcond
  ~JsonString();

  /// Implicitly convertible to string
  /*implicit*/ operator std::string() const { return ToString(); }

  /// Returns a copy of the string
  std::string ToString() const;

  /// Returns a view of the string
  utils::string_view GetView() const;

  const char* Data() const;
  size_t Size() const;

 private:
  static constexpr size_t kSize = 16;
  static constexpr size_t kAlignment = 8;
  utils::FastPimpl<impl::JsonStringImpl, kSize, kAlignment, true> impl_;
};

std::ostream& operator<<(std::ostream&, const JsonString&);

}  // namespace formats::bson
