#include "golden/dto/shop/items.hpp"

#include <userver/chaotic/type_bundle_cpp.hpp>

#include "golden/dto/shop/items_parsers.ipp"

#include "golden/dto/shop/items_sax_parsers.hpp"

namespace golden {namespace dto {

Item FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Item>)
{
    return USERVER_NAMESPACE::formats::json::parser::ParseToType<
        Item,
        USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
            USERVER_NAMESPACE::chaotic::sax::Parser<Item>
        >
    >(json);
}

std::string ToJsonString(const Item& value) {
    USERVER_NAMESPACE::formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

bool operator==(const Item & lhs,const Item & rhs) {
    return true
        && lhs.id == rhs.id
        && lhs.title == rhs.title
        && lhs.last_error == rhs.last_error
    ;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const Item& value)
{
    return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

Item Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Item> to)
{
    return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const Item& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>)
{
    USERVER_NAMESPACE::formats::json::ValueBuilder vb
            = USERVER_NAMESPACE::formats::common::Type::kObject;
    vb["id"] =
        USERVER_NAMESPACE::chaotic::Primitive<int>{
            value.id
        };
    if (value.title) {
        vb["title"] =
            USERVER_NAMESPACE::chaotic::Primitive<std::string>{
                *value.title
            };
    }
    if (value.last_error) {
        vb["last_error"] =
            USERVER_NAMESPACE::chaotic::Primitive<::golden::errors::Error>{
                *value.last_error
            };
    }

    return vb.ExtractValue();
}

void WriteToStream(
    [[maybe_unused]] const Item& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    [[maybe_unused]] bool hide_brackets,
    [[maybe_unused]] std::string_view hide_field_name)
{
    std::optional<USERVER_NAMESPACE::formats::json::StringBuilder::ObjectGuard> guard;
    if (!hide_brackets) {
        guard.emplace(sw);
    }

    if (hide_field_name != "id") {
        sw.Key("id");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<int>{
            value.id
        }, sw);
    }
    if (value.title && hide_field_name != "title") {
        sw.Key("title");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<std::string>{
            *value.title
        }, sw);
    }
    if (value.last_error && hide_field_name != "last_error") {
        sw.Key("last_error");
        WriteToStream(USERVER_NAMESPACE::chaotic::Primitive<::golden::errors::Error>{
            *value.last_error
        }, sw);
    }
}

}  // namespace dto
}  // namespace golden

