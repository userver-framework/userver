#pragma once

#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/user_types.hpp>

namespace storages::postgres::io {

namespace detail {

template <typename UserType, typename WireType, typename Converter,
          DataFormat F, bool Categories>
struct TransformParserBase : BufferParserBase<UserType> {
  using BaseType = BufferParserBase<UserType>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    WireType tmp;
    ReadBuffer<F>(buffer, tmp);
    this->value = Converter{}(tmp);
  }
};

template <typename UserType, typename WireType, typename Converter,
          DataFormat F>
struct TransformParserBase<UserType, WireType, Converter, F, true>
    : BufferParserBase<UserType> {
  using BaseType = BufferParserBase<UserType>;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer,
                  const TypeBufferCategory& categories) {
    WireType tmp;
    ReadBuffer<F>(buffer, tmp, categories);
    this->value = Converter{}(tmp);
  }
};

}  // namespace detail

template <typename UserType, typename WireType, typename Converter,
          DataFormat F>
struct TransformParser : detail::TransformParserBase<
                             UserType, WireType, Converter, F,
                             detail::kParserRequiresTypeCategories<WireType>> {
  using BaseType = detail::TransformParserBase<
      UserType, WireType, Converter, F,
      detail::kParserRequiresTypeCategories<WireType>>;
  using BaseType::BaseType;
};

template <typename UserType, typename WireType, typename Converter,
          DataFormat F>
struct TransformFormatter : detail::BufferFormatterBase<UserType> {
  using BaseType = detail::BufferFormatterBase<UserType>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    WireType tmp = Converter{}(this->value);
    WriteBuffer<F>(types, buffer, tmp);
  }
};
}  // namespace storages::postgres::io
