#pragma once

/// @file userver/formats/json/serialize.hpp
/// @brief Parsers and serializers to/from string and stream

#include <iosfwd>
#include <string_view>

#include <fmt/format.h>

#include <userver/formats/json/value.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

class LogHelper;

}  // namespace logging

namespace formats::json {

/// Parse JSON from string
formats::json::Value FromString(std::string_view doc);

/// Parse JSON from stream
formats::json::Value FromStream(std::istream& is);

/// Serialize JSON to stream
void Serialize(const formats::json::Value& doc, std::ostream& os);

/// Serialize JSON to string
std::string ToString(const formats::json::Value& doc);

/// Stably serialize JSON to string. In result there is no whitespace, keys
/// are sorted and character escaping is stabilized
std::string ToStableString(const formats::json::Value& doc);

std::string ToStableString(formats::json::Value&& doc);

/// Log JSON
logging::LogHelper& operator<<(logging::LogHelper&,
                               const formats::json::Value&);

/// Blocking operations that should not be used on main task processor after
/// startup
namespace blocking {
/// Read JSON from file
formats::json::Value FromFile(const std::string& path);
}  // namespace blocking

namespace impl {

class StringBuffer final {
 public:
  explicit StringBuffer(const formats::json::Value& value);
  ~StringBuffer();

  std::string_view GetStringView() const;

 private:
  struct Impl;
  static constexpr std::size_t kSize = 48;
  static constexpr std::size_t kAlignment = 8;
  utils::FastPimpl<Impl, kSize, kAlignment> pimpl_;
};

}  // namespace impl

}  // namespace formats::json

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::formats::json::Value>
    : fmt::formatter<std::string_view> {
  static auto parse(format_parse_context& ctx) -> decltype(ctx.begin());

  template <typename FormatContext>
  auto format(const USERVER_NAMESPACE::formats::json::Value& value,
              FormatContext& ctx) USERVER_FMT_CONST->decltype(ctx.out()) {
    const USERVER_NAMESPACE::formats::json::impl::StringBuffer buffer(value);
    return formatter<string_view>::format(buffer.GetStringView(), ctx);
  }
};
