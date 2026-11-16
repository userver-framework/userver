#pragma once

#include "golden/dto/shop/items_fwd.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>

#include <golden/errors/errors.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace golden {namespace dto {

struct Item {
    int id{};
    std::optional<std::string> title{};
    std::optional<::golden::errors::Error> last_error{};
};

bool operator==(const Item & lhs,const Item & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Item& value);

Item Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Item>);

Item FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Item>);

std::string ToJsonString(const Item& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Item& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Item& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace dto
}  // namespace golden

