#pragma once

/// @file storages/postgres/io/uuid.hpp
/// @brief UUID I/O support

#include <boost/uuid/uuid.hpp>

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/type_mapping.hpp>

namespace storages::postgres::io {

template <>
struct BufferFormatter<boost::uuids::uuid>
    : detail::BufferFormatterBase<boost::uuids::uuid> {
  using BaseType = detail::BufferFormatterBase<boost::uuids::uuid>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes&, Buffer& buf) const {
    std::copy(value.begin(), value.end(), std::back_inserter(buf));
  }
};

template <>
struct BufferParser<boost::uuids::uuid>
    : detail::BufferParserBase<boost::uuids::uuid> {
  using BaseType = detail::BufferParserBase<boost::uuids::uuid>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buf) {
    if (buf.length != boost::uuids::uuid::static_size()) {
      throw InvalidInputBufferSize{buf.length, "for a uuid value type"};
    }
    std::copy(buf.buffer, buf.buffer + buf.length, value.begin());
  }
};

template <>
struct CppToSystemPg<boost::uuids::uuid>
    : PredefinedOid<PredefinedOids::kUuid> {};

}  // namespace storages::postgres::io
