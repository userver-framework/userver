#include "enum.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "enum_parsers.ipp"

#include "enum_sax_parsers.hpp"

namespace ns {

bool MayParseAs(
    const USERVER_NAMESPACE::formats::json::Value& value,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo>)
{
    return value.IsString() &&
        TryConvert(value.As<std::string>(), USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>{}).has_value();
}

bool MayParseAs(
    const USERVER_NAMESPACE::formats::json::Value& value,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>)
{
    return value.IsString();
}

bool MayParseAs(
    const USERVER_NAMESPACE::formats::json::Value& value,
    USERVER_NAMESPACE::formats::parse::To<Enum>)
{
    if (!value.IsObject() && !value.IsNull()) {
        return false;
    }
    if (const auto field_value = value["foo"];
        !field_value.IsMissing() && !field_value.IsNull() &&
        !USERVER_NAMESPACE::formats::parse::MayParseAs<::ns::Enum::Foo>(field_value)) {
        return false;
    }
    if (const auto field_value = value["with_fallback"];
        !field_value.IsMissing() && !field_value.IsNull() &&
        !USERVER_NAMESPACE::formats::parse::MayParseAs<::ns::Enum::With_Fallback>(field_value)) {
        return false;
    }
    for (auto it = value.begin(), end = value.end(); it != end; ++it) {
        if (!k__ns__Enum_PropertiesNames.Contains(it.GetName())) {
            return false;
        }
    }
    return true;
}

Enum FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Enum>)
{
    return USERVER_NAMESPACE::formats::json::parser::ParseToType<
        Enum,
        USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
            USERVER_NAMESPACE::chaotic::sax::Parser<Enum>
        >
    >(json);
}

std::string ToJsonString(const Enum& value) {
    USERVER_NAMESPACE::formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

bool operator==(const Enum & lhs,const Enum & rhs) {
    return true
        && lhs.foo == rhs.foo
        && lhs.with_fallback == rhs.with_fallback
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const Enum::Foo& value)
{
    return lh << ToString(value);
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const Enum::With_Fallback& value)
{
    return lh << ToString(value);
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const Enum& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

Enum::Foo Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum::With_Fallback Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

Enum::Foo Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum::With_Fallback Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

Enum::Foo Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::Foo> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

Enum::With_Fallback Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

Enum Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Enum> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

Enum::Foo Convert(
      std::string_view value,
      USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>)
{
    const auto result = k__ns__Enum__Foo_Mapping.TryFindBySecond(value);
    if (result.has_value()) {
        return *result;
    }
    throw std::runtime_error(fmt::format("Invalid enum value ({}) for type ::ns::Enum::Foo", value));
}

std::optional<Enum::Foo> TryConvert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>) noexcept
{
    return k__ns__Enum__Foo_Mapping.TryFindBySecond(value);
}

Enum::Foo Parse(std::string_view value, USERVER_NAMESPACE::formats::parse::To<Enum::Foo>)
{
    return Convert(value, USERVER_NAMESPACE::chaotic::convert::To<Enum::Foo>{});
}

Enum::With_Fallback Convert(
      std::string_view value,
      USERVER_NAMESPACE::chaotic::convert::To<Enum::With_Fallback>)
{
    const auto result = k__ns__Enum__With_Fallback_Mapping.TryFindBySecond(value);
    if (result.has_value()) {
        return *result;
    }
    return Enum::With_Fallback::kUnknown;
}

std::optional<Enum::With_Fallback> TryConvert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Enum::With_Fallback>) noexcept
{
    return k__ns__Enum__With_Fallback_Mapping.TryFindBySecond(value);
}

Enum::With_Fallback Parse(std::string_view value, USERVER_NAMESPACE::formats::parse::To<Enum::With_Fallback>)
{
    return Convert(value, USERVER_NAMESPACE::chaotic::convert::To<Enum::With_Fallback>{});
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum::Foo& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    return USERVER_NAMESPACE::formats::json::ValueBuilder(ToString(value)).ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Enum::With_Fallback& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    return USERVER_NAMESPACE::formats::json::ValueBuilder(ToString(value)).ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const Enum& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    if (value.foo) {
        vb["foo"] =
            USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::Foo>{
                *value.foo
            };
    }
    if (value.with_fallback) {
        vb["with_fallback"] =
            USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::With_Fallback>{
                *value.with_fallback
            };
    }

    return vb.ExtractValue();
}
void WriteToStream(
    [[maybe_unused]] const Enum::Foo& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw)
{
        const auto result = k__ns__Enum__Foo_Mapping.TryFindByFirst(value);
        if (result.has_value()) {
            WriteToStream(*result, sw);
        } else {
            throw std::runtime_error("Bad enum value");
        }
}

void WriteToStream(
    [[maybe_unused]] const Enum::With_Fallback& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw)
{
        const auto result = k__ns__Enum__With_Fallback_Mapping.TryFindByFirst(value);
        if (result.has_value()) {
            WriteToStream(*result, sw);
        } else {
            throw std::runtime_error("Bad enum value");
        }
}

void WriteToStream(
    [[maybe_unused]] const Enum& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    [[maybe_unused]] bool hide_brackets,
    [[maybe_unused]] std::string_view hide_field_name)
{
    std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
    if (!hide_brackets) {
        guard.emplace(sw);
    }

    if (value.foo && hide_field_name != "foo") {
        sw.Key("foo");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::Foo>{
            *value.foo
        }, sw);
    }
    if (value.with_fallback && hide_field_name != "with_fallback") {
        sw.Key("with_fallback");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<::ns::Enum::With_Fallback>{
            *value.with_fallback
        }, sw);
    }
}

std::string ToString(Enum::Foo value) {
    const auto result = k__ns__Enum__Foo_Mapping.TryFindByFirst(value);
    if (result.has_value()) {
            return std::string{*result};
    }
    throw std::runtime_error(fmt::format("Invalid enum value: {}", static_cast<int>(value)));
}

std::string ToString(Enum::With_Fallback value) {
    const auto result = k__ns__Enum__With_Fallback_Mapping.TryFindByFirst(value);
    if (result.has_value()) {
            return std::string{*result};
    }
    throw std::runtime_error(fmt::format("Invalid enum value: {}", static_cast<int>(value)));
}

}  // namespace ns

fmt::format_context::iterator fmt::formatter<ns::Enum::Foo>::format(const ns::Enum::Foo& value, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "{}", ToString(value));
}

fmt::format_context::iterator fmt::formatter<ns::Enum::With_Fallback>::format(const ns::Enum::With_Fallback& value, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "{}", ToString(value));
}

