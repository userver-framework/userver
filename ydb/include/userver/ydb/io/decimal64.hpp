#pragma once

/// @file userver/ydb/io/decimal64.hpp
/// @brief YDB serialization support for `userver::decimal64::Decimal`
///
/// `decimal64::Decimal<Prec, RoundPolicy>` is mapped to the YDB type
/// `Decimal(22, Prec)`. The precision is fixed at `22`, which matches the
/// most common "money-like" YDB schema `Decimal(22, 9)` and is sufficient to
/// hold any value representable by `decimal64::Decimal` (its mantissa fits in
/// `int64_t`, i.e. up to 19 significant digits).
///
/// Schemas that use a different precision (e.g. `Decimal(35, 18)`) should use
/// `ydb::Decimal` instead, which carries `precision` and `scale` at runtime.
///
/// On read, `decimal64::Decimal<Prec>::FromStringPermissive` is used, so
/// values stored with a YDB scale larger than `Prec` are rounded according
/// to `RoundPolicy` instead of being rejected.

#include <optional>

#include <ydb-cpp-sdk/client/params/params.h>
#include <ydb-cpp-sdk/client/value/value.h>

#include <userver/compiler/demangle.hpp>
#include <userver/decimal64/decimal64.hpp>
#include <userver/logging/log.hpp>

#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/io/traits.hpp>
#include <userver/ydb/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {

inline bool IsOptionalDecimal64(const NYdb::TValueParser& parser) {
    return parser.GetKind() == NYdb::TTypeParser::ETypeKind::Optional;
}

template <int Prec, typename RoundPolicy>
decimal64::Decimal<Prec, RoundPolicy> ParseDecimal64Value(const NYdb::TValueParser& parser) {
    // YDB stores decimals with a fixed scale, which may differ from `Prec`.
    // `FromStringPermissive` tolerates trailing zeros and rounds extra
    // fractional digits according to `RoundPolicy`, which is what we want.
    return decimal64::Decimal<Prec, RoundPolicy>::FromStringPermissive(parser.GetDecimal().ToString());
}

template <int Prec, typename RoundPolicy, typename Builder>
void WriteDecimal64Value(
    NYdb::TValueBuilderBase<Builder>& builder,
    const decimal64::Decimal<Prec, RoundPolicy>& value
) {
    builder.Decimal(NYdb::TDecimalValue(
        impl::ToString(decimal64::ToString(value)),
        Decimal::kDefaultPrecision,
        static_cast<std::uint8_t>(Prec)
    ));
}

inline NYdb::TType MakeDecimal64Type(std::uint8_t scale) {
    NYdb::TTypeBuilder builder;
    builder.Decimal(NYdb::TDecimalType{Decimal::kDefaultPrecision, scale});
    return builder.Build();
}

inline NYdb::TType MakeOptionalDecimal64Type(std::uint8_t scale) {
    NYdb::TTypeBuilder builder;
    builder.BeginOptional();
    builder.Decimal(NYdb::TDecimalType{Decimal::kDefaultPrecision, scale});
    builder.EndOptional();
    return builder.Build();
}

}  // namespace impl

template <int Prec, typename RoundPolicy>
struct ValueTraits<decimal64::Decimal<Prec, RoundPolicy>> {
    static_assert(
        Prec >= 0 && Prec <= Decimal::kDefaultPrecision,
        "decimal64::Decimal<Prec> must have 0 <= Prec <= ydb::Decimal::kDefaultPrecision (22) "
        "to map to YDB Decimal(22, Prec); use ydb::Decimal for non-money-like schemas"
    );

    using Type = decimal64::Decimal<Prec, RoundPolicy>;

    static Type Parse(NYdb::TValueParser& parser, const ParseContext& /*context*/) {
        const bool is_optional = impl::IsOptionalDecimal64(parser);

        if (is_optional) {
            parser.OpenOptional();
        }

        // Will throw exception if value is null.
        auto value = impl::ParseDecimal64Value<Prec, RoundPolicy>(parser);

        if (is_optional) {
            parser.CloseOptional();
        }

        return value;
    }

    template <typename Builder>
    static void Write(NYdb::TValueBuilderBase<Builder>& builder, const Type& value) {
        impl::WriteDecimal64Value(builder, value);
    }

    static NYdb::TType MakeType() { return impl::MakeDecimal64Type(static_cast<std::uint8_t>(Prec)); }
};

template <int Prec, typename RoundPolicy>
struct ValueTraits<std::optional<decimal64::Decimal<Prec, RoundPolicy>>> {
    static_assert(
        Prec >= 0 && Prec <= Decimal::kDefaultPrecision,
        "decimal64::Decimal<Prec> must have 0 <= Prec <= ydb::Decimal::kDefaultPrecision (22) "
        "to map to YDB Decimal(22, Prec); use ydb::Decimal for non-money-like schemas"
    );

    using Type = decimal64::Decimal<Prec, RoundPolicy>;

    static std::optional<Type> Parse(NYdb::TValueParser& parser, const ParseContext& context) {
        const bool is_optional = impl::IsOptionalDecimal64(parser);
        if (is_optional) {
            parser.OpenOptional();

            if (parser.IsNull()) {
                parser.CloseOptional();
                return {};
            }
        } else {
            LOG_WARNING()
                << "Trying to parse " << context.column_name << " as "
                << compiler::GetTypeName<std::optional<Type>>() << " while actual type is not Optional";
        }

        auto value = impl::ParseDecimal64Value<Prec, RoundPolicy>(parser);
        if (is_optional) {
            parser.CloseOptional();
        }

        return value;
    }

    template <typename Builder>
    static void Write(NYdb::TValueBuilderBase<Builder>& builder, const std::optional<Type>& value) {
        if (value) {
            builder.BeginOptional();
            impl::WriteDecimal64Value(builder, *value);
            builder.EndOptional();
        } else {
            builder.EmptyOptional(impl::MakeDecimal64Type(static_cast<std::uint8_t>(Prec)));
        }
    }

    static NYdb::TType MakeType() { return impl::MakeOptionalDecimal64Type(static_cast<std::uint8_t>(Prec)); }
};

}  // namespace ydb

USERVER_NAMESPACE_END
