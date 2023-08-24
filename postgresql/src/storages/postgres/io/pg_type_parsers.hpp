#pragma once

#include <storages/postgres/internal_pg_types.hpp>
#include <userver/compiler/demangle.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace detail {

template <typename Enum>
struct CharToEnum : BufferParserBase<Enum> {
  using BaseType = BufferParserBase<Enum>;
  using ValueType = typename BaseType::ValueType;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    if (buffer.length != 1) {
      throw InvalidInputBufferSize{
          buffer.length, "for type " + compiler::GetTypeName<ValueType>()};
    }
    this->value = static_cast<ValueType>(*buffer.buffer);
  }
};

}  // namespace detail

template <>
struct BufferParser<DBTypeDescription::TypeClass>
    : detail::CharToEnum<DBTypeDescription::TypeClass> {
  using BaseType = detail::CharToEnum<DBTypeDescription::TypeClass>;
  using BaseType::BaseType;
};

template <>
struct BufferParser<DBTypeDescription::TypeCategory>
    : detail::CharToEnum<DBTypeDescription::TypeCategory> {
  using BaseType = detail::CharToEnum<DBTypeDescription::TypeCategory>;
  using BaseType::BaseType;
};

template <>
struct BufferParser<Oid> : detail::BufferParserBase<Oid> {
  using BufferParserBase::BufferParserBase;

  void operator()(const FieldBuffer& buffer) {
    Integer tmp = 0;
    io::ReadBuffer(buffer, tmp);
    value = tmp;
  }
};

template <>
struct BufferParser<std::uint16_t> : detail::BufferParserBase<std::uint16_t> {
  using BufferParserBase::BufferParserBase;

  void operator()(const FieldBuffer& buffer) {
    Smallint tmp = 0;
    io::ReadBuffer(buffer, tmp);
    value = static_cast<std::uint16_t>(tmp);
  }
};

template <>
struct BufferFormatter<std::uint16_t>
    : detail::BufferFormatterBase<std::uint16_t> {
  using BufferFormatterBase::BufferFormatterBase;
  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    io::WriteBuffer(types, buf, static_cast<std::int16_t>(value));
  }
};

template <>
struct BufferParser<Lsn> : detail::BufferParserBase<Lsn> {
  using BufferParserBase::BufferParserBase;

  void operator()(const FieldBuffer& buffer) {
    Bigint tmp = 0;
    io::ReadBuffer(buffer, tmp);
    value.GetUnderlying() = tmp;
  }
};

template <>
struct CppToSystemPg<Oid> : PredefinedOid<PredefinedOids::kOid> {};
template <>
struct CppToSystemPg<Lsn> : PredefinedOid<PredefinedOids::kLsn> {};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
