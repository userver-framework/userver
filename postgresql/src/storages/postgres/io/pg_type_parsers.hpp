#pragma once

#include <storages/postgres/io/traits.hpp>
#include <utils/demangle.hpp>

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
          buffer.length, "for type " + ::utils::GetTypeName<ValueType>()};
    }
    this->value = static_cast<ValueType>(*buffer.buffer);
  }
};

}  // namespace detail

template <>
struct BufferParser<DBTypeDescription::TypeClass, DataFormat::kBinaryDataFormat>
    : detail::CharToEnum<DBTypeDescription::TypeClass> {};

template <>
struct BufferParser<DBTypeDescription::TypeCategory,
                    DataFormat::kBinaryDataFormat>
    : detail::CharToEnum<DBTypeDescription::TypeCategory> {};

template <>
struct BufferParser<Oid, DataFormat::kBinaryDataFormat>
    : detail::BufferParserBase<Oid> {
  using BufferParserBase::BufferParserBase;

  void operator()(const FieldBuffer& buffer) {
    Integer tmp;
    ReadBuffer<DataFormat::kBinaryDataFormat>(buffer, tmp);
    value = tmp;
  }
};

template <>
struct CppToSystemPg<Oid> : PredefinedOid<PredefinedOids::kOid> {};

}  // namespace storages::postgres::io
