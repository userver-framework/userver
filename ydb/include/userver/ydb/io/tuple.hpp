#pragma once

/// @file userver/ydb/io/tuple.hpp
/// @brief YDB tuple serialization traits

#include <ydb-cpp-sdk/client/value/value.h>

#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include <userver/utils/assert.hpp>
#include <userver/utils/constexpr_indices.hpp>

#include <userver/ydb/io/generic_optional.hpp>
#include <userver/ydb/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {

template <typename Tuple, std::size_t... Indices>
Tuple ParseTupleElements(NYdb::TValueParser& parser, const ParseContext& context, std::index_sequence<Indices...>) {
    return Tuple{([&] {
        const bool has_element = parser.TryNextElement();
        UASSERT(has_element);
        return ydb::Parse<std::tuple_element_t<Indices, Tuple>>(parser, context);
    }())...};
}

template <typename Builder, typename Tuple, std::size_t... Indices>
void WriteTupleElements(NYdb::TValueBuilderBase<Builder>& builder, const Tuple& value, std::index_sequence<Indices...>) {
    (([&] {
         builder.AddElement();
         ydb::Write(builder, std::get<Indices>(value));
     }()),
     ...);
}

}  // namespace impl

template <typename... Args>
struct ValueTraits<std::tuple<Args...>> {
    using TupleType = std::tuple<Args...>;
    static constexpr auto kElementsCount = sizeof...(Args);

    static TupleType Parse(NYdb::TValueParser& parser, const ParseContext& context) {
        parser.OpenTuple();
        auto result = impl::ParseTupleElements<TupleType>(parser, context, std::make_index_sequence<kElementsCount>{});
        parser.CloseTuple();
        return result;
    }

    template <typename Builder>
    static void Write(NYdb::TValueBuilderBase<Builder>& builder, const TupleType& value) {
        builder.BeginTuple();
        impl::WriteTupleElements(builder, value, std::make_index_sequence<kElementsCount>{});
        builder.EndTuple();
    }

    static NYdb::TType MakeType() {
        NYdb::TTypeBuilder builder;
        builder.BeginTuple();
        (builder.AddElement(ValueTraits<Args>::MakeType()), ...);
        builder.EndTuple();
        return builder.Build();
    }
};

template <typename... Args>
struct ValueTraits<std::optional<std::tuple<Args...>>> : impl::GenericOptionalValueTraits<std::tuple<Args...>> {};

}  // namespace ydb

USERVER_NAMESPACE_END
