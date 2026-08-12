#pragma once

#include <string>
#include <type_traits>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/string_builder_fwd.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace utils {
class StringLiteral;
}

namespace chaotic {

namespace impl {
template <typename T>
struct ConstValueRaw {
    using type = T;
};

template <>
struct ConstValueRaw<utils::StringLiteral> {
    using type = std::string;
};
}  // namespace impl

/// @brief A type representing a JSON Schema `const` value.
///
/// Holds no runtime data — all instances are equivalent.
/// Parsing validates that the JSON value equals `Value` and throws
/// `chaotic::Error` if it does not. Serialization always emits `Value`.
template <const auto& Value>
struct ConstValue final {
    using RefValueType = std::remove_cv_t<std::remove_reference_t<decltype(Value)>>;
    using RawValueType = typename impl::ConstValueRaw<RefValueType>::type;

    auto GetValue() const { return Value; }

    operator RefValueType() const { return Value; }

    bool operator==(const ConstValue&) const = default;
};

template <typename RawValue, const auto& Value>
ConstValue<Value> Parse(const RawValue& json, formats::parse::To<ConstValue<Value>>) {
    const auto actual = json.template As<typename ConstValue<Value>::RawValueType>();
    if (actual != Value) {
        chaotic::ThrowForValue(fmt::format("Invalid const value, expected='{}', actual='{}'", Value, actual), json);
    }
    return {};
}

template <typename RawValue, const auto& Value>
RawValue Serialize(const ConstValue<Value>&, formats::serialize::To<RawValue>) {
    return typename RawValue::Builder{Value}.ExtractValue();
}

template <const auto& Value>
void WriteToStream(const ConstValue<Value>&, formats::json::StringBuilder& sw) {
    WriteToStream(Value, sw);
}

}  // namespace chaotic

USERVER_NAMESPACE_END
