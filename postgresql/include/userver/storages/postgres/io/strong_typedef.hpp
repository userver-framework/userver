#pragma once

/// @file userver/storages/postgres/io/strong_typedef.hpp
/// @brief utils::StrongTypedef I/O support
/// @ingroup userver_postgres_parse_and_format

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/nullable_traits.hpp>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace traits {

namespace impl {

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
concept IsStrongTypedefDirectlyMapped =
    IsMappedToUserType<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> ||
    IsMappedToSystemType<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> ||
    IsMappedToArray<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>;

}  // namespace impl

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct IsMappedToPg<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>
    // NOLINTNEXTLINE(google-readability-casting)
    : BoolConstant<impl::IsStrongTypedefDirectlyMapped<Tag, T, Ops> || kIsMappedToPg<T>> {};

// Mark that strong typedef mapping is a special case for disambiguating
// specialization of CppToPg
template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct IsSpecialMapping<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>
    // NOLINTNEXTLINE(google-readability-casting)
    : BoolConstant<!impl::IsStrongTypedefDirectlyMapped<Tag, T, Ops> && kIsMappedToPg<T>> {};

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct IsNullable<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> : IsNullable<T> {};

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct GetSetNull<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> {
    using ValueType = USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>;
    using UnderlyingGetSet = GetSetNull<T>;
    inline static bool IsNull(const ValueType& v) { return UnderlyingGetSet::IsNull(v.GetUnderlying()); }
    inline static void SetNull(ValueType& v) { UnderlyingGetSet::SetNull(v.GetUnderlying()); }
    inline static void SetDefault(ValueType& v) { UnderlyingGetSet::SetDefault(v.GetUnderlying()); }
};

/// A metafunction that enables an enum type serialization to its
/// underlying type. Can be specialized.
template <typename T>
struct CanUseEnumAsStrongTypedef : std::false_type {};

namespace impl {

template <typename T>
constexpr bool CheckCanUseEnumAsStrongTypedef() {
    if constexpr (CanUseEnumAsStrongTypedef<T>{}) {
        static_assert(
            std::is_enum_v<T>,
            "storages::postgres::io::traits::CanUseEnumAsStrongTypedef "
            "should be specialized only for enums"
        );
        static_assert(
            std::is_signed_v<std::underlying_type_t<T>>,
            "storages::postgres::io::traits::CanUseEnumAsStrongTypedef should be "
            "specialized only for enums with signed underlying type"
        );

        return true;
    }

    return false;
}

}  // namespace impl

template <typename T>
concept RequiresCanUseEnumAsStrongTypedef = impl::CheckCanUseEnumAsStrongTypedef<T>();

}  // namespace traits

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct BufferFormatter<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>
    : detail::BufferFormatterBase<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> {
    using BaseType = detail::BufferFormatterBase<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>;
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
struct StrongTypedefParser<StrongTypedef, true> : BufferParserBase<StrongTypedef> {
    using BaseType = BufferParserBase<StrongTypedef>;
    using UnderlyingType = typename StrongTypedef::UnderlyingType;

    using BaseType::BaseType;

    void operator()(const FieldBuffer& buffer, const TypeBufferCategory& categories) {
        UnderlyingType& v = this->value.GetUnderlying();
        io::ReadBuffer(buffer, v, categories);
    }
};

}  // namespace detail

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct BufferParser<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>
    : detail::StrongTypedefParser<
          USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>,
          detail::kParserRequiresTypeCategories<T>> {
    using BaseType = detail::StrongTypedefParser<
        USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>,
        detail::kParserRequiresTypeCategories<T>>;
    using BaseType::BaseType;
};

// StrongTypedef template mapping specialization
template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
requires(!traits::impl::IsStrongTypedefDirectlyMapped<Tag, T, Ops> && traits::kIsMappedToPg<T>)
struct CppToPg<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>> : CppToPg<T> {};

namespace traits {

template <typename Tag, typename T, USERVER_NAMESPACE::utils::StrongTypedefOps Ops>
struct ParserBufferCategory<BufferParser<USERVER_NAMESPACE::utils::StrongTypedef<Tag, T, Ops>>>
    : ParserBufferCategory<typename traits::IO<T>::ParserType> {};

}  // namespace traits

namespace detail {

template <typename T>
struct EnumStrongTypedefFormatter : BufferFormatterBase<T> {
    using BaseType = BufferFormatterBase<T>;
    using BaseType::BaseType;

    template <typename Buffer>
    void operator()(const UserTypes& types, Buffer& buf) const {
        io::WriteBuffer(types, buf, USERVER_NAMESPACE::utils::UnderlyingValue(this->value));
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

template <RequiresCanUseEnumAsStrongTypedef T>
struct Output<T> {
    using type = io::detail::EnumStrongTypedefFormatter<T>;
};

template <RequiresCanUseEnumAsStrongTypedef T>
struct Input<T> {
    using type = io::detail::EnumStrongTypedefParser<T>;
};

template <RequiresCanUseEnumAsStrongTypedef T>
struct IsMappedToPg<T> : std::true_type {};

template <RequiresCanUseEnumAsStrongTypedef T>
struct IsSpecialMapping<T> : std::true_type {};

}  // namespace traits

// enum class strong typedef mapping specialization
template <traits::RequiresCanUseEnumAsStrongTypedef T>
struct CppToPg<T> : CppToPg<std::underlying_type_t<T>> {};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
