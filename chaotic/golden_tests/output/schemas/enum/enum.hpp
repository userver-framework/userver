#pragma once

#include "enum_fwd.hpp"

#include <fmt/core.h>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct Enum {
    enum class Foo {
        kOne,
        kTwo,
        kThree,
    };

    static constexpr Foo kFooValues [] = {
        Foo::kOne,
        Foo::kTwo,
        Foo::kThree,
    };

    enum class With_Fallback {
        kUnknown,
        kOne,
        kTwo,
    };

    static constexpr With_Fallback kWith_FallbackValues [] = {
        With_Fallback::kUnknown,
        With_Fallback::kOne,
        With_Fallback::kTwo,
    };

    std::optional<::ns::Enum::Foo> foo{};
    std::optional<::ns::Enum::With_Fallback> with_fallback{};
};

bool operator==(const Enum & lhs,const Enum & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum::Foo& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum::With_Fallback& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Enum& value);

Enum::Foo Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum::With_Fallback Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>);

Enum Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum::Foo Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum::With_Fallback Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>);

Enum Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum::Foo Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum::With_Fallback Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>);

Enum Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum>);

bool MayParseAs(
    const USERVER_NAMESPACE::formats::json::Value& value,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

bool MayParseAs(
    const USERVER_NAMESPACE::formats::json::Value& value,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>);

bool MayParseAs(
    const USERVER_NAMESPACE::formats::json::Value& value,
    USERVER_NAMESPACE::formats::parse::To<Enum>);

Enum FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Enum>);

std::string ToJsonString(const Enum& value);

Enum::Foo Convert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>);

std::optional<Enum::Foo> TryConvert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>) noexcept;

Enum::Foo Parse(
    std::string_view value,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>);

Enum::With_Fallback Convert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Enum::With_Fallback>);

std::optional<Enum::With_Fallback> TryConvert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Enum::With_Fallback>) noexcept;

Enum::With_Fallback Parse(
    std::string_view value,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum::Foo& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum::With_Fallback& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Enum::Foo& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw);

void WriteToStream(
    const Enum::With_Fallback& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw);

void WriteToStream(
    const Enum& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

std::string ToString(Enum::Foo value);

std::string ToString(Enum::With_Fallback value);

}  // namespace ns

template<>
struct fmt::formatter<ns::Enum::Foo> {
    fmt::format_context::iterator format(const ns::Enum::Foo&, fmt::format_context&) const;

    constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }
};

template<>
struct fmt::formatter<ns::Enum::With_Fallback> {
    fmt::format_context::iterator format(const ns::Enum::With_Fallback&, fmt::format_context&) const;

    constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }
};

