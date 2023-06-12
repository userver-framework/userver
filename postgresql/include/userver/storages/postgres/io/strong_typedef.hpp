#pragma once

/// @file userver/storages/postgres/io/strong_typedef.hpp
/// @brief utils::StrongTypedef I/O support
/// @ingroup userver_postgres_parse_and_format

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/nullable_traits.hpp>

#include <userver/utils/strong_typedef.hpp>
#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

/// @page pg_strong_typedef uPg: support for C++ 'strong typedef' idiom
///
/// Within userver a strong typedef can be expressed as an enum class for
/// integral types and as an instance of
/// `USERVER_NAMESPACE::utils::StrongTypedef` template for all types. Both of
/// them are supported transparently by the PostgresSQL driver with minor
/// limitations. No additional code required.
///
/// @warning The underlying integral type for a strong typedef MUST be
/// signed as PostgreSQL doesn't have unsigned types
///
/// The underlying type for a strong typedef must be mapped to a system or a
/// user PostgreSQL data type. Strong typedef type derives nullability from
/// underlying type.
///
/// Using `USERVER_NAMESPACE::utils::StrongTypedef` example:
/// @snippet storages/postgres/tests/strong_typedef_pgtest.cpp Strong typedef
///
/// Using `enum class` example:
/// @snippet storages/postgres/tests/strong_typedef_pgtest.cpp Enum typedef

namespace traits {

// Helper to detect if the strong typedef mapping is explicitly defined,
// e.g. TimePointTz
template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
inline constexpr bool kIsStrongTypedefDirectlyMapped =
    kIsMappedToUserType<
        USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>> ||
    kIsMappedToSystemType<
        USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>> ||
    kIsMappedToArray<
        USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>;

template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct IsMappedToPg<
    USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : BoolConstant<kIsStrongTypedefDirectlyMapped<Tag, T, Ops, Enable> ||
                   kIsMappedToPg<T>> {};

// Mark that strong typedef mapping is a special case for disambiguating
// specialization of CppToPg
template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct IsSpecialMapping<
    USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : BoolConstant<!kIsStrongTypedefDirectlyMapped<Tag, T, Ops, Enable> &&
                   kIsMappedToPg<T>> {};

template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct IsNullable<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : IsNullable<T> {};

template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct GetSetNull<
    USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>> {
  using ValueType =
      USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>;
  using UnderlyingGetSet = GetSetNull<T>;
  inline static bool IsNull(const ValueType& v) {
    return UnderlyingGetSet::IsNull(v.GetUnderlying());
  }
  inline static void SetNull(ValueType& v) {
    UnderlyingGetSet::SetNull(v.GetUnderlying());
  }
  inline static void SetDefault(ValueType& v) {
    UnderlyingGetSet::SetDefault(v.GetUnderlying());
  }
};

/// A metafunction that enables an enum type serialization to its
/// underlying type. Can be specialized.
template <typename T, typename = USERVER_NAMESPACE::utils::void_t<>>
struct CanUseEnumAsStrongTypedef : std::false_type {};

namespace impl {

template <typename T>
constexpr bool CheckCanUseEnumAsStrongTypedef() {
  if constexpr (CanUseEnumAsStrongTypedef<T>{}) {
    static_assert(std::is_enum_v<T>,
                  "storages::postgres::io::traits::CanUseEnumAsStrongTypedef "
                  "should be specialized only for enums");
    static_assert(
        std::is_signed_v<std::underlying_type_t<T>>,
        "storages::postgres::io::traits::CanUseEnumAsStrongTypedef should be "
        "specialized only for enums with signed underlying type");

    return true;
  }

  return false;
}

}  // namespace impl

template <typename T>
using EnableIfCanUseEnumAsStrongTypedef =
    std::enable_if_t<impl::CheckCanUseEnumAsStrongTypedef<T>()>;

}  // namespace traits

template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct BufferFormatter<
    USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : detail::BufferFormatterBase<
          USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>> {
  using BaseType = detail::BufferFormatterBase<
      USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    io::WriteBuffer(types, buf, this->value.GetUnderlying());
  }
};

namespace detail {
template <typename StrongTypedef, bool Categories = false>
struct StrongTypedefParser : BufferParserBase<StrongTypedef> {
  using BaseType = BufferParserBase<StrongTypedef>;
  using UnderlyingType = typename StrongTypedef::UnderlyingType;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer) {
    UnderlyingType& v = this->value.GetUnderlying();
    io::ReadBuffer(buffer, v);
  }
};

template <typename StrongTypedef>
struct StrongTypedefParser<StrongTypedef, true>
    : BufferParserBase<StrongTypedef> {
  using BaseType = BufferParserBase<StrongTypedef>;
  using UnderlyingType = typename StrongTypedef::UnderlyingType;

  using BaseType::BaseType;

  void operator()(const FieldBuffer& buffer,
                  const TypeBufferCategory& categories) {
    UnderlyingType& v = this->value.GetUnderlying();
    io::ReadBuffer(buffer, v, categories);
  }
};

}  // namespace detail

template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct BufferParser<
    USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>
    : detail::StrongTypedefParser<
          USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>,
          detail::kParserRequiresTypeCategories<T>> {
  using BaseType = detail::StrongTypedefParser<
      USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>,
      detail::kParserRequiresTypeCategories<T>>;
  using BaseType::BaseType;
};

// StrongTypedef template mapping specialization
template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct CppToPg<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>,
               std::enable_if_t<!traits::kIsStrongTypedefDirectlyMapped<
                                    Tag, T, Ops, Enable> &&
                                traits::kIsMappedToPg<T>>> : CppToPg<T> {};

namespace traits {

template <typename Tag, typename T,
          USERVER_NAMESPACE::utils::StrongTypedefOps Ops, typename Enable>
struct ParserBufferCategory<
    BufferParser<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops, Enable>>>
    : ParserBufferCategory<typename traits::IO<T>::ParserType> {};

}  // namespace traits

namespace detail {

template <typename T>
struct EnumStrongTypedefFormatter : BufferFormatterBase<T> {
  using BaseType = BufferFormatterBase<T>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buf) const {
    io::WriteBuffer(types, buf,
                    USERVER_NAMESPACE::utils::UnderlyingValue(this->value));
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
    io::ReadBuffer(buffer, v);
    this->value = static_cast<ValueType>(v);
  }
};

}  // namespace detail

namespace traits {

template <typename T>
struct Output<T, EnableIfCanUseEnumAsStrongTypedef<T>> {
  using type = io::detail::EnumStrongTypedefFormatter<T>;
};

template <typename T>
struct Input<T, EnableIfCanUseEnumAsStrongTypedef<T>> {
  using type = io::detail::EnumStrongTypedefParser<T>;
};

template <typename T>
struct IsMappedToPg<T, EnableIfCanUseEnumAsStrongTypedef<T>> : std::true_type {
};

template <typename T>
struct IsSpecialMapping<T, EnableIfCanUseEnumAsStrongTypedef<T>>
    : std::true_type {};

}  // namespace traits

// enum class strong typedef mapping specialization
template <typename T>
struct CppToPg<T, traits::EnableIfCanUseEnumAsStrongTypedef<T>>
    : CppToPg<std::underlying_type_t<T>> {};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
