#pragma once

/// @file storages/postgres/io/json_types.hpp
/// @brief JSON I/O support

#include <type_traits>

#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/type_traits.hpp>
#include <storages/postgres/io/user_types.hpp>

#include <formats/json.hpp>
#include <utils/strong_typedef.hpp>

namespace storages::postgres {

using PlainJson =
    ::utils::StrongTypedef<struct PlainJsonTag, formats::json::Value,
                           ::utils::StrongTypedefOps::kCompareTransparent>;

namespace io {
namespace detail {

constexpr char kJsonbVersion = 1;

struct JsonParser : BufferParserBase<formats::json::Value> {
  using BaseType = BufferParserBase<formats::json::Value>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer);
};

template <typename JsonValue>
struct JsonFormatter : BufferFormatterBase<JsonValue> {
  using BaseType = BufferFormatterBase<JsonValue>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    using sink_type = boost::iostreams::back_insert_device<Buffer>;
    using stream_type = boost::iostreams::stream<sink_type>;

    if constexpr (!std::is_same_v<PlainJson, JsonValue>) {
      buffer.push_back(kJsonbVersion);
    }
    sink_type sink{buffer};
    stream_type os{sink};
    formats::json::Serialize(
        static_cast<const formats::json::Value&>(this->value), os);
  }
};

}  // namespace detail

namespace traits {

template <>
struct Input<formats::json::Value, DataFormat::kBinaryDataFormat> {
  using type = io::detail::JsonParser;
};

template <>
struct ParserBufferCategory<io::detail::JsonParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<formats::json::Value, DataFormat::kBinaryDataFormat> {
  using type = io::detail::JsonFormatter<formats::json::Value>;
};

template <>
struct Output<PlainJson, DataFormat::kBinaryDataFormat> {
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
