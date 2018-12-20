#pragma once

#include <boost/pfr/precise.hpp>
#include <utility>

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/row_types.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/type_traits.hpp>
#include <storages/postgres/io/user_types.hpp>

#include <utils/variadic_logic.hpp>

namespace storages::postgres::io {

// clang-format wraps snippet lines
// clang-format off
/// @page pg_composite_types µPg: Composite user types
///
/// The driver supports user-defined PostgreSQL composite types. The C++
/// counterpart type must satisfy the same requirements as for the row types,
/// (@ref pg_user_row_types) and must provide a specialisation of CppToUserPg
/// template (@ref pg_user_types).
///
/// Parsing a composite structure from PostgreSQL buffer will throw an error if
/// the number of fields in the postgres data is different from the number of
/// data members in target C++ type. This is the only sanity control for the
/// composite types. The driver doesn't check the data type oids, it's user's
/// responsibility to provide structures with compatible data members.
///
/// @par Examples from tests
///
/// @snippet storages/postgres/tests/composite_types_pg_test.cpp User type declaration
///
/// @warning The type mapping specialisation **must** be accessible at the
/// points where parsing/formatting of the C++ type is instantiated. The
/// header where the C++ type is declared is an appropriate place to do it.
///
/// @snippet storages/postgres/tests/composite_types_pg_test.cpp User type mapping
// clang-format on

namespace detail {

template <typename T>
struct CompositeBinaryParser : BufferParserBase<T> {
  using BaseType = BufferParserBase<T>;
  using RowType = io::RowType<T>;
  using IndexSequence = typename RowType::IndexSequence;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    std::size_t offset{0};

    Integer field_count{0};
    ReadBinary(buffer.GetSubBuffer(offset, int_size), field_count);
    offset += int_size;

    if (field_count != RowType::size) {
      throw CompositeSizeMismatch(field_count, RowType::size);
    }

    ReadTuple(buffer.GetSubBuffer(offset), RowType::GetTuple(this->value),
              IndexSequence{});
  }

 private:
  static constexpr std::size_t int_size = sizeof(Integer);

  template <typename U>
  void ReadField(const FieldBuffer& buffer, std::size_t& offset, U& val) const {
    Integer field_type;
    ReadBinary(buffer.GetSubBuffer(offset, int_size), field_type);
    offset += int_size;
    offset += ReadRawBinary(buffer.GetSubBuffer(offset), val);
  }
  template <typename Tuple, std::size_t... Indexes>
  void ReadTuple(const FieldBuffer& buffer, Tuple&& tuple,
                 std::index_sequence<Indexes...>) const {
    std::size_t offset{0};
    (ReadField(buffer, offset, std::get<Indexes>(std::forward<Tuple>(tuple))),
     ...);
  }
};

template <typename T>
struct CompositeBinaryFormatter : BufferFormatterBase<T> {
  using BaseType = BufferFormatterBase<T>;
  using RowType = io::RowType<T>;
  using IndexSequence = typename RowType::IndexSequence;
  static constexpr std::size_t size = RowType::size;

  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    // Number of fields
    WriteBinary(types, buffer, static_cast<Integer>(size));
    WriteTuple(types, buffer, RowType::GetTuple(this->value), IndexSequence{});
  }

 private:
  template <typename Buffer, typename U>
  void WriteField(const UserTypes& types, Buffer& buffer, const U& val) const {
    Integer field_type = CppToPg<U>::GetOid(types);
    WriteBinary(types, buffer, field_type);
    WriteRawBinary(types, buffer, val);
  }
  template <typename Buffer, typename Tuple, std::size_t... Indexes>
  void WriteTuple(const UserTypes& types, Buffer& buffer, Tuple&& tuple,
                  std::index_sequence<Indexes...>) const {
    (WriteField(types, buffer, std::get<Indexes>(std::forward<Tuple>(tuple))),
     ...);
  }
};

}  // namespace detail

namespace traits {

namespace detail {

template <typename T, DataFormat F>
constexpr bool DetectCompositeParsers() {
  if constexpr (kIsRowType<T>) {
    return TupleHasParsers<typename io::RowType<T>::TupleType, F>::value;
  }
  return false;
}

template <typename T, DataFormat F>
constexpr bool DetectCompositeFormatters() {
  if constexpr (kIsRowType<T>) {
    return TupleHasFormatters<typename io::RowType<T>::TupleType, F>::value;
  }
  return false;
}

template <typename T, DataFormat F>
struct CompositeHasParsers
    : std::integral_constant<bool, DetectCompositeParsers<T, F>()> {};
template <typename T, DataFormat F>
constexpr bool kCompositeHasParsers = CompositeHasParsers<T, F>::value;

template <typename T, DataFormat F>
struct CompositeHasFormatters
    : std::integral_constant<bool, DetectCompositeFormatters<T, F>()> {};
template <typename T, DataFormat F>
constexpr bool kCompositeHasFormatters = CompositeHasFormatters<T, F>::value;

}  // namespace detail

template <typename T>
struct Input<T, DataFormat::kBinaryDataFormat,
             std::enable_if_t<!detail::kCustomBinaryParserDefined<T> &&
                              kIsMappedToUserType<T> && kIsRowType<T>>> {
  static_assert(
      (detail::kCompositeHasParsers<T, DataFormat::kBinaryDataFormat> == true),
      "Not all composite type members have binary parsers");
  using type = io::detail::CompositeBinaryParser<T>;
};

template <typename T>
struct Output<T, DataFormat::kBinaryDataFormat,
              std::enable_if_t<!detail::kCustomBinaryFormatterDefined<T> &&
                               kIsMappedToUserType<T> && kIsRowType<T>>> {
  static_assert(
      (detail::kCompositeHasFormatters<T, DataFormat::kBinaryDataFormat> ==
       true),
      "Not all composite type members have binary formatters");
  using type = io::detail::CompositeBinaryFormatter<T>;
};

}  // namespace traits
}  // namespace storages::postgres::io
