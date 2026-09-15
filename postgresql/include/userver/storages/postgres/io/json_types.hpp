#pragma once

/// @file userver/storages/postgres/io/json_types.hpp
/// @brief JSON I/O support
/// @ingroup userver_postgres_parse_and_format

#include <type_traits>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <userver/formats/json/raw_string.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/strong_typedef.hpp>

// TODO: remove this include
#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

using PlainJson = USERVER_NAMESPACE::utils::StrongTypedef<
    struct PlainJsonTag,
    formats::json::Value,
    USERVER_NAMESPACE::utils::StrongTypedefOps::kCompareTransparent>;

namespace io {
namespace detail {

inline constexpr char kJsonbVersion = 1;

struct JsonParser : BufferParserBase<formats::json::Value> {
    using BaseType = BufferParserBase<formats::json::Value>;
    using BaseType::BaseType;

    void operator()(const FieldBuffer& buffer);
};

struct JsonRawStringParser : BufferParserBase<formats::json::RawString> {
    using BaseType = BufferParserBase<formats::json::RawString>;
    using BaseType::BaseType;

    void operator()(const FieldBuffer& buffer);
};

void JsonValueToBuffer(const formats::json::Value& value, std::vector<char>& buffer);

void JsonValueToBuffer(const formats::json::Value& value, std::string& buffer);

template <typename JsonValue>
struct JsonFormatter : BufferFormatterBase<JsonValue> {
    using BaseType = BufferFormatterBase<JsonValue>;
    using BaseType::BaseType;

    template <typename Buffer>
    void operator()(const UserTypes&, Buffer& buffer) const {
        if constexpr (!std::is_same_v<PlainJson, JsonValue>) {
            buffer.push_back(kJsonbVersion);
        }

        detail::JsonValueToBuffer(static_cast<const formats::json::Value&>(this->value), buffer);
    }
};

struct JsonRawStringFormatter : BufferFormatterBase<formats::json::RawString> {
    using BaseType = BufferFormatterBase<formats::json::RawString>;
    using BaseType::BaseType;

    template <typename Buffer>
    void operator()(const UserTypes&, Buffer& buffer) const {
        WriteAsJsonb(buffer);
    }

    template <typename Buffer>
    void operator()(const UserTypes&, Buffer& buffer, Oid replace_oid) const {
        if (replace_oid == static_cast<Oid>(PredefinedOids::kJson)) {
            throw InvalidInputFormat{
                "formats::json::RawString binds as jsonb and cannot be written to a PostgreSQL json "
                "field; explicitly cast to the expected type in SQL query (e.g. `$1::json`)"
            };
        }
        WriteAsJsonb(buffer);
    }

private:
    template <typename Buffer>
    void WriteAsJsonb(Buffer& buffer) const {
        const auto view = this->value.GetView();
        if constexpr (traits::CanReserve<Buffer>) {
            buffer.reserve(buffer.size() + 1 + view.size());
        }
        buffer.push_back(kJsonbVersion);
        buffer.insert(buffer.end(), view.begin(), view.end());
    }
};

}  // namespace detail

namespace traits {

template <>
struct Input<formats::json::Value> {
    using type = io::detail::JsonParser;
};

template <>
struct Input<formats::json::RawString> {
    using type = io::detail::JsonRawStringParser;
};

template <>
struct ParserBufferCategory<io::detail::JsonParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct ParserBufferCategory<io::detail::JsonRawStringParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<formats::json::Value> {
    using type = io::detail::JsonFormatter<formats::json::Value>;
};

template <>
struct Output<PlainJson> {
    using type = io::detail::JsonFormatter<PlainJson>;
};

template <>
struct Output<formats::json::RawString> {
    using type = io::detail::JsonRawStringFormatter;
};

}  // namespace traits

template <>
struct CppToSystemPg<formats::json::Value> : PredefinedOid<PredefinedOids::kJsonb> {};

template <>
struct CppToSystemPg<formats::json::RawString> : PredefinedOid<PredefinedOids::kJsonb> {};

template <>
struct CppToSystemPg<PlainJson> : PredefinedOid<PredefinedOids::kJson> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
