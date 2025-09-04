#pragma once

#include <ydb-cpp-sdk/client/value/value.h>

#include <optional>

#include <userver/ydb/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

template <typename T>
struct GenericOptionalValueTraits {
    static std::optional<T> Parse(NYdb::TValueParser& parser, const ParseContext& context) {
        std::optional<T> result;
        parser.OpenOptional();
        if (!parser.IsNull()) {
            result.emplace(ValueTraits<T>::Parse(parser, context));
        }
        parser.CloseOptional();
        return result;
    }

    template <typename Builder, typename U = const std::optional<T>&>
    static void Write(NYdb::TValueBuilderBase<Builder>& builder, U&& value) {
        if (value.has_value()) {
            builder.BeginOptional();
            ValueTraits<T>::Write(builder, std::forward<U>(value).value());
            builder.EndOptional();
        } else {
            builder.EmptyOptional(ValueTraits<T>::MakeType());
        }
    }

    static NYdb::TType MakeType() {
        NYdb::TTypeBuilder builder;
        builder.Optional(ValueTraits<T>::MakeType());
        return builder.Build();
    }
};

}  // namespace ydb::impl

USERVER_NAMESPACE_END
