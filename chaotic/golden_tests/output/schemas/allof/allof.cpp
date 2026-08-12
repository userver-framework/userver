#include "allof.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "allof_parsers.ipp"

#include "allof_sax_parsers.hpp"

namespace ns {

AllOf FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<AllOf>)
{
    return USERVER_NAMESPACE::formats::json::parser::ParseToType<
        AllOf,
        USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
            USERVER_NAMESPACE::chaotic::sax::Parser<AllOf>
        >
    >(json);
}

std::string ToJsonString(const AllOf& value) {
    USERVER_NAMESPACE::formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

bool operator==(const AllOf::Foo__P0 & lhs,const AllOf::Foo__P0 & rhs) {
    return true
        && lhs.foo == rhs.foo
        && lhs.extra == rhs.extra
    ;
}

bool operator==(const AllOf::Foo__P1 & lhs,const AllOf::Foo__P1 & rhs) {
    return true
        && lhs.bar == rhs.bar
        && lhs.extra == rhs.extra
    ;
}

bool operator==(const AllOf::Foo & lhs,const AllOf::Foo & rhs) {
    return
            static_cast<const AllOf::Foo__P0&>(lhs) ==
            static_cast<const AllOf::Foo__P0&>(rhs)
&&                     static_cast<const AllOf::Foo__P1&>(lhs) ==
            static_cast<const AllOf::Foo__P1&>(rhs)
        ;
}

bool operator==(const AllOf & lhs,const AllOf & rhs) {
    return true
        && lhs.foo == rhs.foo
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const AllOf::Foo__P0& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const AllOf::Foo__P1& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const AllOf::Foo& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const AllOf& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

AllOf::Foo__P0 Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P0> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf::Foo__P1 Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P1> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf::Foo Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

AllOf::Foo__P0 Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P0> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

AllOf::Foo__P1 Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P1> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

AllOf::Foo Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

AllOf Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

AllOf::Foo__P0 Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P0> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

AllOf::Foo__P1 Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P1> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

AllOf::Foo Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf::Foo> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

AllOf Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<AllOf> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const AllOf::Foo__P0& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = value.extra;
    if (value.foo) {
        vb["foo"] =
            USERVER_NAMESPACE::chaotic::Primitive<std::string>{
                *value.foo
            };
    }

    return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const AllOf::Foo__P1& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = value.extra;
    if (value.bar) {
        vb["bar"] =
            USERVER_NAMESPACE::chaotic::Primitive<int>{
                *value.bar
            };
    }

    return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    const AllOf::Foo& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

        USERVER_NAMESPACE::formats::common::Merge(
            vb,
            USERVER_NAMESPACE::formats::json::ValueBuilder{static_cast<AllOf::Foo__P0>(value)}.ExtractValue()
        );
        USERVER_NAMESPACE::formats::common::Merge(
            vb,
            USERVER_NAMESPACE::formats::json::ValueBuilder{static_cast<AllOf::Foo__P1>(value)}.ExtractValue()
        );

    return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const AllOf& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    if (value.foo) {
        vb["foo"] =
            USERVER_NAMESPACE::chaotic::Primitive<::ns::AllOf::Foo>{
                *value.foo
            };
    }

    return vb.ExtractValue();
}

void WriteToStream(
    [[maybe_unused]] const AllOf::Foo__P0& value,
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
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{
            *value.foo
        }, sw);
    }
    for (const auto& [field_key, field_value]: USERVER_NAMESPACE::formats::common::Items(value.extra)) {
        sw.Key(field_key);
        WriteToStream(field_value, sw);
    }
}

void WriteToStream(
    [[maybe_unused]] const AllOf::Foo__P1& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    [[maybe_unused]] bool hide_brackets,
    [[maybe_unused]] std::string_view hide_field_name)
{
    std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
    if (!hide_brackets) {
        guard.emplace(sw);
    }

    if (value.bar && hide_field_name != "bar") {
        sw.Key("bar");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{
            *value.bar
        }, sw);
    }
    for (const auto& [field_key, field_value]: USERVER_NAMESPACE::formats::common::Items(value.extra)) {
        sw.Key(field_key);
        WriteToStream(field_value, sw);
    }
}

void WriteToStream(
    [[maybe_unused]] const AllOf::Foo& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    [[maybe_unused]] bool hide_brackets,
    [[maybe_unused]] std::string_view hide_field_name)
{
    std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
    if (!hide_brackets) {
        guard.emplace(sw);
    }

        USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

    if (value.foo && hide_field_name != "foo") {
        sw.Key("foo");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{
            *value.foo
        }, sw);
    }
                    USERVER_NAMESPACE::formats::common::Merge(vb, static_cast<const ::ns::AllOf::Foo__P0&>(value).extra);

    if (value.bar && hide_field_name != "bar") {
        sw.Key("bar");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{
            *value.bar
        }, sw);
    }
                    USERVER_NAMESPACE::formats::common::Merge(vb, static_cast<const ::ns::AllOf::Foo__P1&>(value).extra);

            auto additional_properties = vb.ExtractValue();
            for (const auto& [name, item]: USERVER_NAMESPACE::formats::common::Items(additional_properties)) {
                sw.Key(name);
                WriteToStream(item, sw);
            }
}

void WriteToStream(
    [[maybe_unused]] const AllOf& value,
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
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<::ns::AllOf::Foo>{
            *value.foo
        }, sw);
    }
}

}  // namespace ns

