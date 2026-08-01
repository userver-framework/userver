#pragma once

/// @file userver/storages/odbc/impl/parameter.hpp
/// @brief Internal storage for ODBC query parameters.

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::impl {

enum class ParameterType {
    kBoolean,
    kSignedInteger,
    kUnsignedInteger,
    kFloatingPoint,
    kString,
    kUnknown,
};

/// A type-erased, owning query parameter. Owning the value is important because
/// an ODBC driver is allowed to read bound buffers until SQLExecute returns.
class Parameter final {
public:
    using Value = std::variant<bool, std::int64_t, std::uint64_t, double, std::string>;

    Parameter(std::nullptr_t)
        : type_{ParameterType::kUnknown},
          is_null_{true},
          value_{std::string{}}
    {}
    Parameter(std::nullopt_t)
        : Parameter{nullptr}
    {}

    Parameter(bool value)
        : type_{ParameterType::kBoolean},
          value_{value}
    {}

    template <std::signed_integral T>
    requires(!std::same_as<T, bool>)
    Parameter(T value)
        : type_{ParameterType::kSignedInteger},
          value_{static_cast<std::int64_t>(value)}
    {}

    template <std::unsigned_integral T>
    requires(!std::same_as<T, bool>)
    Parameter(T value)
        : type_{ParameterType::kUnsignedInteger},
          value_{static_cast<std::uint64_t>(value)}
    {}

    template <std::floating_point T>
    Parameter(T value)
        : type_{ParameterType::kFloatingPoint},
          value_{static_cast<double>(value)}
    {}

    template <typename T>
    requires std::is_enum_v<T>
    Parameter(T value)
        : Parameter{static_cast<std::underlying_type_t<T>>(value)}
    {}

    Parameter(const char* value)
        : type_{ParameterType::kString},
          is_null_{value == nullptr},
          value_{value == nullptr ? std::string{} : std::string{value}}
    {}
    Parameter(std::string value)
        : type_{ParameterType::kString},
          value_{std::move(value)}
    {}
    Parameter(std::string_view value)
        : Parameter{std::string{value}}
    {}

    template <typename T>
    Parameter(const std::optional<T>& value)
        : Parameter{value ? Parameter{*value} : NullOf<T>()}
    {}

    ParameterType GetType() const noexcept { return type_; }
    bool IsNull() const noexcept { return is_null_; }

    template <typename T>
    const T& Get() const {
        return std::get<T>(value_);
    }

private:
    template <typename T>
    static Parameter NullOf() {
        Parameter result{T{}};
        result.is_null_ = true;
        return result;
    }

    ParameterType type_;
    bool is_null_{false};
    Value value_;
};

using ParameterList = std::vector<Parameter>;

template <typename... Args>
ParameterList MakeParameterList(const Args&... args) {
    ParameterList result;
    result.reserve(sizeof...(Args));
    (result.emplace_back(args), ...);
    return result;
}

}  // namespace storages::odbc::impl

USERVER_NAMESPACE_END
