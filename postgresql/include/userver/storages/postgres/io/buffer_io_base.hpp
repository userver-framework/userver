#pragma once

#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::detail {

template <typename T>
inline constexpr bool kShouldInitMapping = requires { T::init; };

template <typename T>
struct BufferParserBase {
    using ValueType = T;

    ValueType& value;
    explicit BufferParserBase(ValueType& v)
        : value{v}
    {
        using PgMapping = CppToPg<ValueType>;
        if constexpr (kShouldInitMapping<PgMapping>) {
            ForceReference(PgMapping::init);
        }
    }
};

template <typename T>
struct BufferParserBase<T&&> {
    using ValueType = T;

    ValueType value;
    explicit BufferParserBase(ValueType&& v)
        : value{std::move(v)}
    {
        using PgMapping = CppToPg<ValueType>;
        if constexpr (kShouldInitMapping<PgMapping>) {
            ForceReference(PgMapping::init);
        }
    }
};

template <typename T>
struct BufferFormatterBase {
    using ValueType = T;
    const ValueType& value;
    explicit BufferFormatterBase(const ValueType& v)
        : value{v}
    {}
};

}  // namespace storages::postgres::io::detail

USERVER_NAMESPACE_END
