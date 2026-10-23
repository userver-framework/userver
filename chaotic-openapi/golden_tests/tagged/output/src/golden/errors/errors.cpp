#include "golden/errors/errors.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "golden/errors/errors_parsers.ipp"

#include "golden/errors/errors_sax_parsers.hpp"

namespace golden {namespace errors {

Error FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Error>)
{
    return USERVER_NAMESPACE::formats::json::parser::ParseToType<
        Error,
        USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
            USERVER_NAMESPACE::chaotic::sax::Parser<Error>
        >
    >(json);
}

std::string ToJsonString(const Error& value) {
    USERVER_NAMESPACE::formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

bool operator==(const Error & lhs,const Error & rhs) {
    return true
        && lhs.code == rhs.code
        && lhs.message == rhs.message
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const Error& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

Error Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Error> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const Error& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    vb["code"] =
        USERVER_NAMESPACE::chaotic::Primitive<std::string>{
            value.code
        };
    if (value.message) {
        vb["message"] =
            USERVER_NAMESPACE::chaotic::Primitive<std::string>{
                *value.message
            };
    }

    return vb.ExtractValue();
}

void WriteToStream(
    [[maybe_unused]] const Error& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    [[maybe_unused]] bool hide_brackets,
    [[maybe_unused]] std::string_view hide_field_name)
{
    std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
    if (!hide_brackets) {
        guard.emplace(sw);
    }

    if (hide_field_name != "code") {
        sw.Key("code");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{
            value.code
        }, sw);
    }
    if (value.message && hide_field_name != "message") {
        sw.Key("message");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{
            *value.message
        }, sw);
    }
}

}  // namespace errors
}  // namespace golden

