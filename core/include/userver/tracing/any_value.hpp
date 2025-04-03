#pragma once

/// @file userver/tracing/any_value.hpp
/// @brief @copybrief tracing::AnyValue

#include <cstdint>
#include <string>
#include <variant>

#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

/// @see
/// https://github.com/open-telemetry/opentelemetry-proto/blob/v1.3.2/opentelemetry/proto/common/v1/common.proto#L28
class AnyValue {
    using Data = std::variant<std::string, bool, std::int64_t, double>;

public:
    explicit AnyValue(std::string string_value);
    explicit AnyValue(const char* string_value);
    explicit AnyValue(bool bool_value);
    explicit AnyValue(int int_value);
    explicit AnyValue(long int_value);
    explicit AnyValue(long long int_value);
    explicit AnyValue(unsigned int int_value);
    explicit AnyValue(unsigned long int_value);
    explicit AnyValue(unsigned long long int_value);
    explicit AnyValue(float double_value);
    explicit AnyValue(double double_value);

    /// @cond
    const Data& GetData() const;
    /// @endcond

private:
    Data value_;
};

AnyValue Parse(const formats::json::Value& value, formats::parse::To<AnyValue>);

void WriteToStream(const AnyValue& any_value, formats::json::StringBuilder& sw);

}  // namespace tracing

USERVER_NAMESPACE_END
