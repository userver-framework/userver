#pragma once

/// @file userver/storages/postgres/io/uuid.hpp
/// @brief UUID I/O support

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

namespace boost::uuids {
struct uuid;
}

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

template <>
struct BufferFormatter<boost::uuids::uuid>
    : detail::BufferFormatterBase<boost::uuids::uuid> {
  using BaseType = detail::BufferFormatterBase<boost::uuids::uuid>;
  using BaseType::BaseType;

  void operator()(const UserTypes&, std::vector<char>& buf) const;
};

template <>
struct BufferParser<boost::uuids::uuid>
    : detail::BufferParserBase<boost::uuids::uuid> {
  using BaseType = detail::BufferParserBase<boost::uuids::uuid>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buf);
};

template <>
struct CppToSystemPg<boost::uuids::uuid>
    : PredefinedOid<PredefinedOids::kUuid> {};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
