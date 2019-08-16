#pragma once

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

namespace storages::postgres::io {

namespace detail {

struct JsonParser : BufferParserBase<formats::json::Value> {
  using BaseType = BufferParserBase<formats::json::Value>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer);
};

struct JsonFormatter : BufferFormatterBase<formats::json::Value> {
  using BaseType = BufferFormatterBase<formats::json::Value>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    using sink_type = boost::iostreams::back_insert_device<Buffer>;
    using stream_type = boost::iostreams::stream<sink_type>;

    sink_type sink{buffer};
    stream_type os{sink};
    formats::json::Serialize(value, os);
  }
};

}  // namespace detail

namespace traits {

template <>
struct Input<formats::json::Value, DataFormat::kBinaryDataFormat> {
  using type = io::detail::JsonParser;
};

template <>
struct Output<formats::json::Value, DataFormat::kBinaryDataFormat> {
  using type = io::detail::JsonFormatter;
};

}  // namespace traits

template <>
struct CppToSystemPg<formats::json::Value>
    : PredefinedOid<PredefinedOids::kJson> {};

}  // namespace storages::postgres::io
