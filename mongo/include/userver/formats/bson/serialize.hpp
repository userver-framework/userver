#pragma once

/// @file userver/formats/bson/serialize.hpp
/// @brief Textual serialization helpers

#include <iosfwd>
#include <string>
#include <string_view>

#include <fmt/core.h>

#include <userver/compiler/select.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/value.hpp>
#include <userver/formats/json_fwd.hpp>
#include <userver/logging/log_helper_fwd.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

formats::json::Value Convert(const formats::bson::Value& bson,
                             formats::parse::To<formats::json::Value>);
formats::bson::Value Convert(const formats::json::Value& json,
                             formats::parse::To<formats::bson::Value>);

}  // namespace formats::parse

namespace formats::bson {

/// Wraps To*JsonString results to avoid unneeded copying
class JsonString;

/// Applies heuristics to convert JSON string to BSON document.
/// As JSON have rather lax typing, some heuristics are applied to guess correct
/// BSON types for values. It is strongly recommended to write your own
/// conversion routines matching your schemas.
/// @warning Stability of heuristics is not guaranteed, this is provided as-is.
Document FromJsonString(std::string_view json);

/// Applies heuristics to convert JSON string to BSON array.
/// As JSON have rather lax typing, some heuristics are applied to guess correct
/// BSON types for values. It is strongly recommended to write your own
/// conversion routines matching your schemas.
/// @warning Stability of heuristics is not guaranteed, this is provided as-is.
Value ArrayFromJsonString(std::string_view json);

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

/// Converts BSON array to a legacy libbson's JSON format, except the outermost
/// element is encoded as a JSON array, rather than a JSON document.
JsonString ToArrayJsonString(const formats::bson::Value&);

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
  std::string_view GetView() const;

  const char* Data() const;
  size_t Size() const;

 private:
  static constexpr std::size_t kSize = compiler::SelectSize()  //
                                           .For64Bit(16)
                                           .For32Bit(8);
  static constexpr std::size_t kAlignment = alignof(void*);
  utils::FastPimpl<impl::JsonStringImpl, kSize, kAlignment, utils::kStrictMatch>
      impl_;
};

std::ostream& operator<<(std::ostream&, const JsonString&);

logging::LogHelper& operator<<(logging::LogHelper&, const JsonString&);

/// Uses formats::bson::ToRelaxedJsonString representation by default.
logging::LogHelper& operator<<(logging::LogHelper&, const Document&);

}  // namespace formats::bson

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::formats::bson::JsonString>
    : public fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const USERVER_NAMESPACE::formats::bson::JsonString& json,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::formatter<std::string_view>::format(json.GetView(), ctx);
  }
};

/// Uses formats::bson::ToRelaxedJsonString representation by default.
template <>
struct fmt::formatter<USERVER_NAMESPACE::formats::bson::Document>
    : public fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const USERVER_NAMESPACE::formats::bson::Document& bson,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::formatter<std::string_view>::format(
        USERVER_NAMESPACE::formats::bson::ToRelaxedJsonString(bson).GetView(),
        ctx);
  }
};
