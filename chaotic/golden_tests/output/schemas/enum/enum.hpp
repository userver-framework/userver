#pragma once

#include <fmt/core.h>

#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/type_bundle_hpp.hpp>

#include "enum_fwd.hpp"

namespace ns {

struct Enum {
  enum class Foo {
    kOne,
    kTwo,
    kThree,
  };

  static constexpr Foo kFooValues[] = {
      Foo::kOne,
      Foo::kTwo,
      Foo::kThree,
  };

  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefoo = "foo";
  std::optional<::ns::Enum::Foo> foo{};
};

bool operator==(const Enum& lhs, const Enum& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum::Foo& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum& value);

Enum::Foo Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum::Foo Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum::Foo Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum::Foo Convert(std::string_view value, USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>);

std::optional<Enum::Foo> TryConvert(std::string_view value,
                                    USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>) noexcept;

Enum::Foo Parse(std::string_view value, USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum::Foo& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

std::string ToString(Enum::Foo value);

}  // namespace ns

template <>
struct fmt::formatter<ns::Enum::Foo> {
  fmt::format_context::iterator format(const ns::Enum::Foo&, fmt::format_context&) const;

  constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
};

