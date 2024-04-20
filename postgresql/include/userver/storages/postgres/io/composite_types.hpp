#pragma once

/// @file userver/storages/postgres/io/composite_types.hpp
/// @brief Composite types I/O support
/// @ingroup userver_postgres_parse_and_format

#include <boost/pfr/core.hpp>
#include <utility>

#include <userver/compiler/demangle.hpp>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/row_types.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace detail {

void InitRecordParser();

template <typename T>
struct CompositeBinaryParser : BufferParserBase<T> {
  using BaseType = BufferParserBase<T>;
  using RowType = io::RowType<T>;
  using IndexSequence = typename RowType::IndexSequence;

  using BaseType::BaseType;

  void operator()(FieldBuffer buffer, const TypeBufferCategory& categories) {
    if constexpr (!traits::kIsMappedToUserType<T>) {
      InitRecordParser();
    }

    Integer field_count{0};
    buffer.Read(field_count, BufferCategory::kPlainBuffer);

    if (field_count != RowType::size) {
      throw CompositeSizeMismatch(field_count, RowType::size,
                                  compiler::GetTypeName<T>());
    }

    ReadTuple(buffer, categories, RowType::GetTuple(this->value),
              IndexSequence{});
  }

 private:
  static constexpr std::size_t int_size = sizeof(Integer);

  template <typename U>
  void ReadField(FieldBuffer& buffer, const TypeBufferCategory& categories,
                 U& val) const {
    Integer field_type = 0;
    buffer.Read(field_type, BufferCategory::kPlainBuffer);
    auto elem_category = GetTypeBufferCategory(categories, field_type);
    if (elem_category == BufferCategory::kNoParser) {
      throw LogicError{"Buffer category for oid " + std::to_string(field_type) +
                       " is unknown"};
    }
    buffer.ReadRaw(val, categories, elem_category);
  }
  template <typename Tuple, std::size_t... Indexes>
  void ReadTuple(FieldBuffer& buffer, const TypeBufferCategory& categories,
                 Tuple&& tuple, std::index_sequence<Indexes...>) const {
    (ReadField(buffer, categories,
               std::get<Indexes>(std::forward<Tuple>(tuple))),
     ...);
  }
};

template <typename T>
struct CompositeBinaryFormatter : BufferFormatterBase<T> {
  using BaseType = BufferFormatterBase<T>;
  using PgMapping = CppToPg<T>;
  using RowType = io::RowType<T>;
  using IndexSequence = typename RowType::IndexSequence;
  static constexpr std::size_t size = RowType::size;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    const auto& type_desc =
        types.GetCompositeDescription(PgMapping::GetOid(types));
    if (type_desc.Size() != size) {
      throw CompositeSizeMismatch{type_desc.Size(), size,
                                  compiler::GetTypeName<T>()};
    }
    // Number of fields
    io::WriteBuffer(types, buffer, static_cast<Integer>(size));
    WriteTuple(types, type_desc, buffer, RowType::GetTuple(this->value),
               IndexSequence{});
  }

 private:
  template <typename Buffer, typename U>
  void WriteField(const UserTypes& types,
                  const CompositeTypeDescription& type_desc, std::size_t index,
                  Buffer& buffer, const U& val) const {
    auto field_type = CppToPg<U>::GetOid(types);
    const auto& field_desc = type_desc[index];
    if (field_type != field_desc.type) {
      if (io::MappedToSameType(static_cast<PredefinedOids>(field_type),
                               static_cast<PredefinedOids>(
                                   types.FindDomainBaseOid(field_desc.type)))) {
        field_type = field_desc.type;
      } else {
        throw CompositeMemberTypeMismatch(
            PgMapping::postgres_name.schema, PgMapping::postgres_name.name,
            field_desc.name, field_desc.type, field_type);
      }
    }
    io::WriteBuffer(types, buffer, static_cast<Integer>(field_type));
    io::WriteRawBinary(types, buffer, val, field_type);
  }
  template <typename Buffer, typename Tuple, std::size_t... Indexes>
  void WriteTuple(const UserTypes& types,
                  const CompositeTypeDescription& type_desc, Buffer& buffer,
                  Tuple&& tuple, std::index_sequence<Indexes...>) const {
    (WriteField(types, type_desc, Indexes, buffer,
                std::get<Indexes>(std::forward<Tuple>(tuple))),
     ...);
  }
};

}  // namespace detail

namespace traits {

namespace detail {

template <typename Tuple>
struct AssertTupleHasParsers;

template <typename... Members>
struct AssertTupleHasParsers<std::tuple<Members...>> : std::true_type {
  static_assert((HasParser<std::decay_t<Members>>::value && ...),
                "No parser for member. Probably you forgot to include "
                "file with parser or to define your own. Please see page "
                "`uPg: Supported data types` for more information");
};

template <typename Tuple>
struct AssertTupleHasFormatters;

template <typename... Members>
struct AssertTupleHasFormatters<std::tuple<Members...>> : std::true_type {
  static_assert(
      (HasFormatter<std::decay_t<Members>>::value && ...),
      "No formatter for member. Probably you forgot to "
      "include file with formatter or to define your own. Please see page "
      "`uPg: Supported data types` for more information");
};

template <typename T>
constexpr bool AssertHasCompositeParsers() {
  io::traits::AssertIsValidRowType<T>();
  return AssertTupleHasParsers<typename io::RowType<T>::TupleType>::value;
}

template <typename T>
constexpr bool AssertHasCompositeFormatters() {
  io::traits::AssertIsValidRowType<T>();
  return AssertTupleHasFormatters<typename io::RowType<T>::TupleType>::value;
}

}  // namespace detail

template <typename T>
struct Input<
    T, std::enable_if_t<!detail::kCustomParserDefined<T> && kIsRowType<T>>> {
  static_assert(detail::AssertHasCompositeParsers<T>());
  using type = io::detail::CompositeBinaryParser<T>;
};

template <typename T>
struct Output<T, std::enable_if_t<!detail::kCustomFormatterDefined<T> &&
                                  kIsMappedToUserType<T> && kIsRowType<T>>> {
  static_assert(detail::AssertHasCompositeFormatters<T>());
  using type = io::detail::CompositeBinaryFormatter<T>;
};

template <typename T>
struct ParserBufferCategory<io::detail::CompositeBinaryParser<T>>
    : std::integral_constant<BufferCategory, BufferCategory::kCompositeBuffer> {
};

}  // namespace traits
}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
