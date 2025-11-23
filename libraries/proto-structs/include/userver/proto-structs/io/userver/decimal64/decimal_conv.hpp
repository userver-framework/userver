#pragma once

/// @file userver/proto-structs/io/userver/decimal64/decimal64_conv.hpp
/// @brief Provides read/write context class with the ability to handle `userver::decimal64::Decimal` conversion.

#include <userver/proto-structs/io/userver/decimal64/decimal.hpp>

#include <google/type/decimal.pb.h>

#include <userver/proto-structs/io/context.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::io {

template <int Prec, typename TRoundPolicy>
decimal64::Decimal<Prec, TRoundPolicy> ReadProtoStruct(
    ReadContext& ctx,
    To<decimal64::Decimal<Prec, TRoundPolicy>>,
    const ::google::type::Decimal& msg
) {
    using Decimal = decimal64::Decimal<Prec, TRoundPolicy>;
    Decimal result = {};

    try {
        result = Decimal(msg.value());
    } catch (const decimal64::ParseError& e) {
        ctx.AddError(fmt::format(
            "'{}' does not represent a valid value for decimal with {} precision, parse error '{}'",
            msg.value(),
            Prec,
            e.what()
        ));
    }

    return result;
}

template <int Prec, typename TRoundPolicy>
void WriteProtoStruct(WriteContext&, const decimal64::Decimal<Prec, TRoundPolicy>& obj, ::google::type::Decimal& msg) {
    msg.set_value(ToString(obj));
}

}  // namespace proto_structs::io

USERVER_NAMESPACE_END
