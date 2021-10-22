#pragma once

#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

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
    if constexpr (traits::kIsMappedToSystemType<UserType> &&
                  traits::kIsMappedToSystemType<WireType>) {
      static_assert(CppToPg<UserType>::type_oid == CppToPg<WireType>::type_oid,
                    "Database type mismatch in TransformFormatter");
    }
    UASSERT_MSG(
        CppToPg<UserType>::GetOid(types) == CppToPg<WireType>::GetOid(types),
        "Database type mismatch in TransformFormatter");

    WireType tmp = Converter{}(this->value);
    io::WriteBuffer(types, buffer, tmp);
  }
};
}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
