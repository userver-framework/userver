#include "header_value.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <fmt/format.h>

#include <userver/formats/common/items.hpp>
#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

namespace {

template <typename T>
HeaderValue MakeHeaderValue(T&& value) {
    return HeaderValue::Builder{std::forward<T>(value)}.ExtractValue();
}

AMQP::Array ToAmqpArray(const HeaderValue& value);
AMQP::Table ToAmqpTable(const HeaderValue& value);

template <typename Func>
decltype(auto) WithAmqpField(const HeaderValue& value, Func&& func) {
    if (value.IsNull()) {
        return std::forward<Func>(func)(AMQP::VoidField{});
    }
    if (value.IsBool()) {
        return std::forward<Func>(func)(AMQP::BooleanSet{value.As<bool>()});
    }
    if (value.IsInt()) {
        return std::forward<Func>(func)(AMQP::Long{value.As<int>()});
    }
    if (value.IsInt64()) {
        return std::forward<Func>(func)(AMQP::LongLong{value.As<std::int64_t>()});
    }
    if (value.IsUInt()) {
        return std::forward<Func>(func)(AMQP::ULong{value.As<unsigned int>()});
    }
    if (value.IsUInt64()) {
        return std::forward<Func>(func)(AMQP::ULongLong{value.As<std::uint64_t>()});
    }
    if (value.IsDouble()) {
        return std::forward<Func>(func)(AMQP::Double{value.As<double>()});
    }
    if (value.IsString()) {
        return std::forward<Func>(func)(AMQP::LongString{value.As<std::string>()});
    }
    if (value.IsArray()) {
        auto array = ToAmqpArray(value);
        return std::forward<Func>(func)(array);
    }
    if (value.IsObject()) {
        auto table = ToAmqpTable(value);
        return std::forward<Func>(func)(table);
    }

    throw std::runtime_error{fmt::format("Unsupported RabbitMQ header value at '{}'", value.GetPath())};
}

HeaderValue ToHeaderValueFromArray(const AMQP::Array& array) {
    HeaderValue::Builder builder{formats::common::Type::kArray};
    for (std::uint32_t index = 0; index < array.count(); ++index) {
        builder.PushBack(impl::ToHeaderValue(array[index]));
    }

    return builder.ExtractValue();
}

AMQP::Array ToAmqpArray(const HeaderValue& value) {
    AMQP::Array array;
    for (const auto& item : value) {
        WithAmqpField(item, [&array](const AMQP::Field& field) { array.push_back(field); });
    }

    return array;
}

AMQP::Table ToAmqpTable(const HeaderValue& value) {
    AMQP::Table table;
    for (const auto& [key, item] : formats::common::Items(value)) {
        const auto key_copy = std::string{key};
        WithAmqpField(item, [&table, key_copy](const AMQP::Field& field) { table.set(key_copy, field); });
    }

    return table;
}

HeaderValue ToHeaderValueFromTable(const AMQP::Table& table) {
    HeaderValue::Builder builder{formats::common::Type::kObject};
    for (const auto& key : table.keys()) {
        builder.EmplaceNocheck(key, impl::ToHeaderValue(table[key]));
    }

    return builder.ExtractValue();
}

}  // namespace

HeaderValue ToHeaderValue(const AMQP::Field& field) {
    switch (field.typeID()) {
        case 'S':
        case 's':
            return MakeHeaderValue(static_cast<const std::string&>(field));
        case 't':
            return MakeHeaderValue(static_cast<const AMQP::BooleanSet&>(field).value() != 0);
        case 'B':
            return MakeHeaderValue(static_cast<unsigned int>(field));
        case 'b':
            return MakeHeaderValue(static_cast<int>(field));
        case 'u':
            return MakeHeaderValue(static_cast<unsigned int>(field));
        case 'U':
            return MakeHeaderValue(static_cast<int>(field));
        case 'i':
            return MakeHeaderValue(static_cast<unsigned int>(field));
        case 'I':
            return MakeHeaderValue(static_cast<int>(field));
        case 'l':
        case 'T':
            return MakeHeaderValue(static_cast<std::uint64_t>(field));
        case 'L':
            return MakeHeaderValue(static_cast<std::int64_t>(field));
        case 'f':
            return MakeHeaderValue(static_cast<double>(static_cast<float>(field)));
        case 'd':
        case 'D':
            return MakeHeaderValue(static_cast<double>(field));
        case 'A':
            return ToHeaderValueFromArray(static_cast<const AMQP::Array&>(field));
        case 'F':
            return ToHeaderValueFromTable(static_cast<const AMQP::Table&>(field));
        case 'V':
            return HeaderValue::Builder{}.ExtractValue();
    }

    throw std::runtime_error{fmt::format("Unsupported AMQP header field type '{}'", field.typeID())};
}

std::unordered_map<std::string, HeaderValue> TableToHeaders(const AMQP::Table& table) {
    const auto keys = table.keys();

    std::unordered_map<std::string, HeaderValue> headers;
    headers.reserve(keys.size());

    for (const auto& key : keys) {
        headers.emplace(key, ToHeaderValue(table[key]));
    }

    return headers;
}

void AddHeadersToTable(AMQP::Table& table, const std::unordered_map<std::string, HeaderValue>& headers) {
    for (const auto& [key, value] : headers) {
        const auto key_copy = key;
        WithAmqpField(value, [&table, key_copy](const AMQP::Field& field) { table.set(key_copy, field); });
    }
}

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
