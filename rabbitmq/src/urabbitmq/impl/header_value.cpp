#include "header_value.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include <userver/formats/common/items.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

formats::json::Value ToJsonValue(const AMQP::Array& array);
formats::json::Value ToJsonValue(const AMQP::Table& table);
std::unique_ptr<AMQP::Field> ToAmqpField(const HeaderValue& value);

[[noreturn]] void ThrowUnsupportedAmqpField(char type_id) {
    throw std::runtime_error{fmt::format("Unsupported AMQP header field type '{}'", type_id)};
}

[[noreturn]] void ThrowUnsupportedHeaderValue(const HeaderValue& value) {
    throw std::runtime_error{fmt::format("Unsupported RabbitMQ header value at '{}'", value.GetPath())};
}

formats::json::Value ToJsonValue(const AMQP::Array& array) {
    formats::json::ValueBuilder builder{formats::common::Type::kArray};
    const auto count = array.count();
    for (std::uint32_t index = 0; index < count; ++index) {
        builder.PushBack(formats::json::ValueBuilder{FieldToHeaderValue(array[static_cast<std::uint8_t>(index)])});
    }

    return builder.ExtractValue();
}

formats::json::Value ToJsonValue(const AMQP::Table& table) {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    for (const auto& key : table.keys()) {
        builder.EmplaceNocheck(key, formats::json::ValueBuilder{FieldToHeaderValue(table[key])});
    }

    return builder.ExtractValue();
}

AMQP::Array ToAmqpArray(const HeaderValue& value) {
    AMQP::Array array;
    for (const auto& item : value) {
        array.push_back(*ToAmqpField(item));
    }

    return array;
}

AMQP::Table ToAmqpTable(const HeaderValue& value) {
    AMQP::Table table;
    for (const auto& [key, item] : formats::common::Items(value)) {
        table.set(key, *ToAmqpField(item));
    }

    return table;
}

std::unique_ptr<AMQP::Field> ToAmqpField(const HeaderValue& value) {
    if (value.IsNull()) {
        return std::make_unique<AMQP::VoidField>();
    }
    if (value.IsBool()) {
        return std::make_unique<AMQP::BooleanSet>(value.As<bool>());
    }
    if (value.IsInt() || value.IsInt64()) {
        return std::make_unique<AMQP::LongLong>(value.As<std::int64_t>());
    }
    if (value.IsUInt() || value.IsUInt64()) {
        return std::make_unique<AMQP::ULongLong>(value.As<std::uint64_t>());
    }
    if (value.IsDouble()) {
        return std::make_unique<AMQP::Double>(value.As<double>());
    }
    if (value.IsString()) {
        return std::make_unique<AMQP::LongString>(value.As<std::string>());
    }
    if (value.IsArray()) {
        return std::make_unique<AMQP::Array>(ToAmqpArray(value));
    }
    if (value.IsObject()) {
        return std::make_unique<AMQP::Table>(ToAmqpTable(value));
    }

    ThrowUnsupportedHeaderValue(value);
}

}  // namespace

HeaderValue FieldToHeaderValue(const AMQP::Field& field) {
    switch (field.typeID()) {
        case 'S':
        case 's':
            return formats::json::ValueBuilder{static_cast<const std::string&>(field)}.ExtractValue();
        case 't':
            return formats::json::ValueBuilder{static_cast<const AMQP::BooleanSet&>(field).value() != 0}.ExtractValue();
        case 'B':
            return formats::json::ValueBuilder{static_cast<std::uint64_t>(static_cast<std::uint8_t>(field))}
                .ExtractValue();
        case 'b':
            return formats::json::ValueBuilder{static_cast<std::int64_t>(static_cast<std::int8_t>(field))}
                .ExtractValue();
        case 'u':
            return formats::json::ValueBuilder{static_cast<std::uint64_t>(static_cast<std::uint16_t>(field))}
                .ExtractValue();
        case 'U':
            return formats::json::ValueBuilder{static_cast<std::int64_t>(static_cast<std::int16_t>(field))}
                .ExtractValue();
        case 'i':
            return formats::json::ValueBuilder{static_cast<std::uint64_t>(static_cast<std::uint32_t>(field))}
                .ExtractValue();
        case 'I':
            return formats::json::ValueBuilder{static_cast<std::int64_t>(static_cast<std::int32_t>(field))}
                .ExtractValue();
        case 'l':
        case 'T':
            return formats::json::ValueBuilder{static_cast<std::uint64_t>(field)}.ExtractValue();
        case 'L':
            return formats::json::ValueBuilder{static_cast<std::int64_t>(field)}.ExtractValue();
        case 'f':
            return formats::json::ValueBuilder{static_cast<double>(static_cast<float>(field))}.ExtractValue();
        case 'd':
        case 'D':
            return formats::json::ValueBuilder{static_cast<double>(field)}.ExtractValue();
        case 'A':
            return ToJsonValue(static_cast<const AMQP::Array&>(field));
        case 'F':
            return ToJsonValue(static_cast<const AMQP::Table&>(field));
        case 'V':
            return formats::json::ValueBuilder{}.ExtractValue();
    }

    ThrowUnsupportedAmqpField(field.typeID());
}

std::unordered_map<std::string, HeaderValue> TableToHeaders(const AMQP::Table& table) {
    const auto keys = table.keys();

    std::unordered_map<std::string, HeaderValue> headers;
    headers.reserve(keys.size());

    for (const auto& key : keys) {
        headers.emplace(key, FieldToHeaderValue(table[key]));
    }

    return headers;
}

void AddHeadersToTable(AMQP::Table& table, const std::unordered_map<std::string, HeaderValue>& headers) {
    for (const auto& [key, value] : headers) {
        table.set(key, *ToAmqpField(value));
    }
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
