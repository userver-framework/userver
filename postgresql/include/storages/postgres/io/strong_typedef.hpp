#pragma once

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>

#include <utils/strong_typedef.hpp>
#include <utils/void_t.hpp>

namespace storages::postgres::io {

/// @page pg_strong_typedef µPg: support for C++ 'strong typedef' idiom
///
/// Within µserver a strong typedef can be expressed as an enum class for
/// integral types and as an instance of `::utils::StrongTypedef` template for
/// all types. Both of them are supported transparently by the PostgresSQL
/// driver with minor limitations. No additional code required.
///
/// @warning The underlying integral type for a strong typedef MUST be
/// signed as PostgreSQL doesn't have unsigned types
///
/// The underlying type for a strong typedef must be mapped to a system or a
/// user PostgreSQL data type. Strong typedef type derives nullability from
/// underlying type.
///
/// Using `::utils::StrongTypedef` example:
/// @snippet storages/postgres/tests/strong_typedef_pg_test.cpp Strong typedef
///
/// Using `enum class` example:
/// @snippet storages/postgres/tests/strong_typedef_pg_test.cpp Enum typedef

namespace traits {

// Helper to detect if the strong typedef mapping is explicitly defined,
// e.g. TimePointTz
template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
constexpr bool kIsStrongTypedefDirectlyMapped =
    kIsMappedToUserType<::utils::StrongTypedef<Tag, T, Ops, Enable>> ||
    kIsMappedToSystemType<::utils::StrongTypedef<Tag, T, Ops, Enable>> ||
    kIsMappedToArray<::utils::StrongTypedef<Tag, T, Ops, Enable>>;

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct IsMappedToPg<::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : BoolConstant<kIsStrongTypedefDirectlyMapped<Tag, T, Ops, Enable> ||
                   kIsMappedToPg<T>> {};

// Mark that strong typedef mapping is a special case for disambiguating
// specialisation of CppToPg
template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct IsSpecialMapping<::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : BoolConstant<!kIsStrongTypedefDirectlyMapped<Tag, T, Ops, Enable> &&
                   kIsMappedToPg<T>> {};

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct IsNullable<::utils::StrongTypedef<Tag, T, Ops, Enable>> : IsNullable<T> {
};

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct GetSetNull<::utils::StrongTypedef<Tag, T, Ops, Enable>> {
  using ValueType = ::utils::StrongTypedef<Tag, T, Ops, Enable>;
  using UnderlyingGetSet = GetSetNull<T>;
  inline static bool IsNull(const ValueType& v) {
    return UnderlyingGetSet::IsNull(v.GetUnderlying());
  }
  inline static void SetNull(ValueType& v) {
    UnderlyingGetSet::SetNull(v.GetUnderlying());
  }
  inline static void SetDefault(ValueType& v) {
    UnderlyingGetSet::SetNull(v.GetUnderlying());
  }
};

// A metafunction that checks that an enum type is NOT mapped to a user PG type
// and it's underlying type is signed
template <typename T, typename = ::utils::void_t<>>
struct CanUseEnumAsStrongTypedef : std::false_type {};

template <typename T>
struct CanUseEnumAsStrongTypedef<
    T, std::enable_if_t<std::is_enum<T>() && !kIsMappedToUserType<T>>>
    // signedness of value is checked in derivation clause because instantiating
    // std::underlying_type with non-enum type is a hard error
    : std::is_signed<std::underlying_type_t<T>> {};

template <typename T>
using EnableIfCanUseEnumAsStrongTypedef =
    std::enable_if_t<CanUseEnumAsStrongTypedef<T>{}>;

}  // namespace traits

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct BufferFormatter<::utils::StrongTypedef<Tag, T, Ops, Enable>,
                       DataFormat::kBinaryDataFormat>
    : detail::BufferFormatterBase<::utils::StrongTypedef<Tag, T, Ops, Enable>> {
  using BaseType =
      detail::BufferFormatterBase<::utils::StrongTypedef<Tag, T, Ops, Enable>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    WriteBinary(types, buf, this->value.GetUnderlying());
  }
};

template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct BufferParser<::utils::StrongTypedef<Tag, T, Ops, Enable>,
                    DataFormat::kBinaryDataFormat>
    : detail::BufferParserBase<::utils::StrongTypedef<Tag, T, Ops, Enable>> {
  using BaseType =
      detail::BufferParserBase<::utils::StrongTypedef<Tag, T, Ops, Enable>>;
  using ValueType = typename BaseType::ValueType;
  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    typename ValueType::UnderlyingType& v = this->value.GetUnderlying();
    ReadBinary(buffer, v);
  }
};

// StrongTypedef template mapping specialisation
template <typename Tag, typename T, ::utils::StrongTypedefOps Ops,
          typename Enable>
struct CppToPg<::utils::StrongTypedef<Tag, T, Ops, Enable>,
               std::enable_if_t<!traits::kIsStrongTypedefDirectlyMapped<
                                    Tag, T, Ops, Enable> &&
                                traits::kIsMappedToPg<T>>> : CppToPg<T> {};

namespace detail {

template <typename T>
struct EnumStrongTypedefFormatter : BufferFormatterBase<T> {
  using BaseType = BufferFormatterBase<T>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    WriteBinary(types, buf, ::utils::UnderlyingValue(this->value));
  }
};

template <typename T>
struct EnumStrongTypedefParser : BufferParserBase<T> {
  using BaseType = BufferParserBase<T>;
  using ValueType = typename BaseType::ValueType;
  using UnderlyingType = std::underlying_type_t<ValueType>;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    UnderlyingType v;
    ReadBinary(buffer, v);
    this->value = static_cast<ValueType>(v);
  }
};

}  // namespace detail

namespace traits {

template <typename T>
struct Output<T, DataFormat::kBinaryDataFormat,
              EnableIfCanUseEnumAsStrongTypedef<T>> {
  using type = io::detail::EnumStrongTypedefFormatter<T>;
};

template <typename T>
struct Input<T, DataFormat::kBinaryDataFormat,
             EnableIfCanUseEnumAsStrongTypedef<T>> {
  using type = io::detail::EnumStrongTypedefParser<T>;
};

template <typename T>
struct IsMappedToPg<T, EnableIfCanUseEnumAsStrongTypedef<T>> : std::true_type {
};

template <typename T>
struct IsSpecialMapping<T, EnableIfCanUseEnumAsStrongTypedef<T>>
    : std::true_type {};

}  // namespace traits

// enum class strong typedef mapping specialisation
template <typename T>
struct CppToPg<T, traits::EnableIfCanUseEnumAsStrongTypedef<T>>
    : CppToPg<std::underlying_type_t<T>> {};

}  // namespace storages::postgres::io
