#include "any.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "any_parsers.ipp"

#include "any_sax_parsers.hpp"

namespace ns {

WithAnyField FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField>)
{
    return USERVER_NAMESPACE::formats::json::parser::ParseToType<
        WithAnyField,
        USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
            USERVER_NAMESPACE::chaotic::sax::Parser<WithAnyField>
        >
    >(json);
}

std::string ToJsonString(const WithAnyField& value) {
    USERVER_NAMESPACE::formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

bool operator==(const WithAnyField & lhs,const WithAnyField & rhs) {
    return true
        && lhs.payload == rhs.payload
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const WithAnyField& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

WithAnyField Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

WithAnyField Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField> to)
{
    return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

WithAnyField Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<WithAnyField> to)
{
    return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const WithAnyField& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    if (value.payload) {
        vb["payload"] =
            USERVER_NAMESPACE::formats::json::Value{
                *value.payload
            };
    }

    return vb.ExtractValue();
}

void WriteToStream(
    [[maybe_unused]] const WithAnyField& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    [[maybe_unused]] bool hide_brackets,
    [[maybe_unused]] std::string_view hide_field_name)
{
    std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
    if (!hide_brackets) {
        guard.emplace(sw);
    }

    if (value.payload && hide_field_name != "payload") {
        sw.Key("payload");
        WriteToStream(USERVER_NAMESPACE::formats::json::Value{
            *value.payload
        }, sw);
    }
}

}  // namespace ns

