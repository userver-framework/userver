#pragma once

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/user_types.hpp>

namespace storages::postgres::io {

namespace detail {

template <typename UserType, typename WireType, typename Converter,
          bool Categories>
struct TransformParserBase : BufferParserBase<UserType> {
  using BaseType = BufferParserBase<UserType>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    WireType tmp;
    io::ReadBuffer(buffer, tmp);
    this->value = Converter{}(tmp);
  }
};

template <typename UserType, typename WireType, typename Converter>
struct TransformParserBase<UserType, WireType, Converter, true>
    : BufferParserBase<UserType> {
  using BaseType = BufferParserBase<UserType>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer,
                  const TypeBufferCategory& categories) {
    WireType tmp;
    io::ReadBuffer(buffer, tmp, categories);
    this->value = Converter{}(tmp);
  }
};

}  // namespace detail

template <typename UserType, typename WireType, typename Converter>
struct TransformParser : detail::TransformParserBase<
                             UserType, WireType, Converter,
                             detail::kParserRequiresTypeCategories<WireType>> {
  using BaseType = detail::TransformParserBase<
      UserType, WireType, Converter,
      detail::kParserRequiresTypeCategories<WireType>>;
  using BaseType::BaseType;
};

template <typename UserType, typename WireType, typename Converter>
struct TransformFormatter : detail::BufferFormatterBase<UserType> {
  using BaseType = detail::BufferFormatterBase<UserType>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WireType tmp = Converter{}(this->value);
    io::WriteBuffer(types, buffer, tmp);
  }
};
}  // namespace storages::postgres::io
