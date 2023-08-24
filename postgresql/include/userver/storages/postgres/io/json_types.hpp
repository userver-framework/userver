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

#include <userver/formats/json.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

using PlainJson = USERVER_NAMESPACE::utils::StrongTypedef<
    struct PlainJsonTag, formats::json::Value,
    USERVER_NAMESPACE::utils::StrongTypedefOps::kCompareTransparent>;

namespace io {
namespace detail {

inline constexpr char kJsonbVersion = 1;

struct JsonParser : BufferParserBase<formats::json::Value> {
  using BaseType = BufferParserBase<formats::json::Value>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer);
};

void JsonValueToBuffer(const formats::json::Value& value,
                       std::vector<char>& buffer);

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

    detail::JsonValueToBuffer(
        static_cast<const formats::json::Value&>(this->value), buffer);
  }
};

}  // namespace detail

namespace traits {

template <>
struct Input<formats::json::Value> {
  using type = io::detail::JsonParser;
};

template <>
struct ParserBufferCategory<io::detail::JsonParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<formats::json::Value> {
  using type = io::detail::JsonFormatter<formats::json::Value>;
};

template <>
struct Output<PlainJson> {
  using type = io::detail::JsonFormatter<PlainJson>;
};

}  // namespace traits

template <>
struct CppToSystemPg<formats::json::Value>
    : PredefinedOid<PredefinedOids::kJsonb> {};

template <>
struct CppToSystemPg<PlainJson> : PredefinedOid<PredefinedOids::kJson> {};

}  // namespace io
}  // namespace storages::postgres

USERVER_NAMESPACE_END
