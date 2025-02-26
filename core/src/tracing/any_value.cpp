#include <userver/tracing/any_value.hpp>

#include <fmt/format.h>

#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/serialize/write_to_stream.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/numeric_cast.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

enum ValueIndex : std::uint8_t {
    String = 0,
    Bool = 1,
    Int = 2,
    Double = 3,
};

utils::StringLiteral ToString(ValueIndex index) {
    switch (index) {
        case String:
            return "String";
        case Bool:
            return "Bool";
        case Int:
            return "Int";
        case Double:
            return "Double";
    }

    UINVARIANT(false, fmt::format("Unexpected index: {}", static_cast<std::uint8_t>(index)));
}

template <ValueIndex expected>
void CheckIndex(std::uint8_t actual) {
    UASSERT_MSG(
        actual == expected,
        fmt::format("Expected: '{}', Actual: '{}'", ToString(expected), ToString(static_cast<ValueIndex>(actual)))
    );
}

}  // namespace

AnyValue::AnyValue(std::string string_value) : value_{std::move(string_value)} {
    CheckIndex<ValueIndex::String>(value_.index());
}

AnyValue::AnyValue(const char* string_value) : value_{std::string{string_value}} {
    CheckIndex<ValueIndex::String>(value_.index());
}

AnyValue::AnyValue(bool bool_value) : value_{bool_value} { CheckIndex<ValueIndex::Bool>(value_.index()); }

AnyValue::AnyValue(int int_value) : value_{utils::numeric_cast<std::int64_t>(int_value)} {
    CheckIndex<ValueIndex::Int>(value_.index());
}

AnyValue::AnyValue(long int_value) : value_{utils::numeric_cast<std::int64_t>(int_value)} {
    CheckIndex<ValueIndex::Int>(value_.index());
}

AnyValue::AnyValue(long long int_value) : value_{utils::numeric_cast<std::int64_t>(int_value)} {
    CheckIndex<ValueIndex::Int>(value_.index());
}

AnyValue::AnyValue(unsigned int int_value) : value_{utils::numeric_cast<std::int64_t>(int_value)} {
    CheckIndex<ValueIndex::Int>(value_.index());
}

AnyValue::AnyValue(unsigned long int_value) : value_{utils::numeric_cast<std::int64_t>(int_value)} {
    CheckIndex<ValueIndex::Int>(value_.index());
}

AnyValue::AnyValue(unsigned long long int_value) : value_{utils::numeric_cast<std::int64_t>(int_value)} {
    CheckIndex<ValueIndex::Int>(value_.index());
}

AnyValue::AnyValue(float double_value) : value_{static_cast<double>(double_value)} {
    CheckIndex<ValueIndex::Double>(value_.index());
}

AnyValue::AnyValue(double double_value) : value_{double_value} { CheckIndex<ValueIndex::Double>(value_.index()); }

const AnyValue::Data& AnyValue::GetData() const { return value_; }

AnyValue Parse(const formats::json::Value& value, formats::parse::To<AnyValue>) {
    if (value.IsString()) {
        return AnyValue{value.As<std::string>()};
    } else if (value.IsInt()) {  // must be before is double, because integer is also double
        return AnyValue{value.As<std::int64_t>()};
    } else if (value.IsDouble()) {
        return AnyValue{value.As<double>()};
    } else if (value.IsBool()) {
        return AnyValue{value.As<bool>()};
    }

    UINVARIANT(false, fmt::format("Unexpected attribute value: {}", value));
}

void WriteToStream(const AnyValue& any_value, formats::json::StringBuilder& sw) {
    USERVER_NAMESPACE::formats::serialize::WriteToStream(any_value.GetData(), sw);
}

}  // namespace tracing

USERVER_NAMESPACE_END
