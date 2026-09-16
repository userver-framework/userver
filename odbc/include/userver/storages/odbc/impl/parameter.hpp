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

#include <boost/pfr/core.hpp>
#include <boost/pfr/traits.hpp>

#include <userver/storages/odbc/io/type_mapping.hpp>
#include <userver/storages/odbc/types.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::impl {

template <typename T>
struct IsNativeParameterValue {
private:
    using Value = std::remove_cvref_t<T>;
    using Pointee = std::remove_pointer_t<Value>;
    using Element = std::remove_extent_t<Value>;

public:
    static constexpr bool value =
        std::integral<Value> || std::floating_point<Value> || std::is_enum_v<Value> ||
        std::same_as<Value, std::string> || std::same_as<Value, std::string_view> || std::same_as<Value, Bytes> ||
        std::same_as<Value, Date> || std::same_as<Value, Time> || std::same_as<Value, Timestamp> || kIsDecimal<Value> ||
        std::same_as<Value, std::nullptr_t> || std::same_as<Value, std::nullopt_t> ||
        (std::is_pointer_v<Value> && (std::same_as<Pointee, char> || std::same_as<Pointee, const char>)) ||
        (std::is_array_v<Value> && std::same_as<std::remove_cv_t<Element>, char>);
};

template <typename T>
inline constexpr bool kIsNativeParameterValue = IsNativeParameterValue<T>::value;

template <typename T>
constexpr bool IsParameterScalarValue() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (io::traits::kIsOptional<Value>) {
        if constexpr (io::traits::kHasMappingDeclaration<Value>) {
            return false;
        } else {
            using Inner = typename io::traits::IsOptional<Value>::ValueType;
            return !io::traits::kIsOptional<Inner> && IsParameterScalarValue<Inner>();
        }
    } else if constexpr (kIsNativeParameterValue<Value> && !std::is_enum_v<Value>) {
        // Native non-enum behavior cannot be shadowed by a user mapping.
        return true;
    } else if constexpr (io::traits::kHasMappingDeclaration<Value>) {
        // A declared mapping suppresses enum and aggregate fallbacks even when
        // the mapping is malformed or lacks the input direction.
        return io::traits::kHasToOdbc<Value>;
    } else {
        return kIsNativeParameterValue<Value>;
    }
}

template <typename T>
inline constexpr bool kIsParameterStoreValue = IsParameterScalarValue<T>();

struct OdbcParameterMappingTag;

template <typename T, std::size_t... Index>
constexpr bool AreParameterMembersMappable(std::index_sequence<Index...>) {
    return sizeof...(Index) != 0 &&
           ((!std::is_reference_v<boost::pfr::tuple_element_t<Index, T>> &&
             kIsParameterStoreValue<boost::pfr::tuple_element_t<Index, T>>) &&
            ...);
}

template <typename T>
constexpr bool DetectParameterAggregate() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (io::traits::kIsOptional<Value> || io::traits::kHasMappingDeclaration<Value> ||
                  !std::is_class_v<Value> || !std::is_aggregate_v<Value> || !std::is_standard_layout_v<Value> ||
                  std::is_union_v<Value> || !io::traits::kAggregateHasNoBaseClass<Value> ||
                  !boost::pfr::is_implicitly_reflectable_v<Value, OdbcParameterMappingTag>)
    {
        return false;
    } else {
        return AreParameterMembersMappable<Value>(std::make_index_sequence<boost::pfr::tuple_size_v<Value>>{});
    }
}

template <typename T>
inline constexpr bool kIsParameterAggregate = DetectParameterAggregate<T>();

template <typename T>
inline constexpr bool kIsParameterArgument = kIsParameterStoreValue<T> || kIsParameterAggregate<T>;

enum class ParameterType {
    kBoolean,
    kSignedInteger,
    kUnsignedInteger,
    kFloatingPoint,
    kString,
    kBytes,
    kDate,
    kTime,
    kTimestamp,
    kDecimal,
    kUnknown,
};

struct DecimalParameter final {
    std::string representation;
    std::uint8_t precision;
    std::uint8_t scale;
};

/// A type-erased, owning query parameter. Owning the value is important because
/// an ODBC driver is allowed to read bound buffers until SQLExecute returns.
class Parameter final {
public:
    using Value = std::variant<
        bool,
        std::int64_t,
        std::uint64_t,
        double,
        std::string,
        Bytes,
        Date,
        Time,
        Timestamp,
        DecimalParameter>;

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
    requires(std::is_enum_v<T> && !io::traits::kHasMappingDeclaration<T>)
    Parameter(T value)
        : Parameter{static_cast<std::underlying_type_t<T>>(value)}
    {}

    template <typename T>
    requires(io::traits::kHasToOdbc<T> && !io::traits::kIsDirectBoundType<std::remove_cvref_t<T>>)
    Parameter(const T& value)
        : Parameter{io::CppToOdbc<std::remove_cvref_t<T>>::ToOdbc(value)}
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

    Parameter(Bytes value)
        : type_{ParameterType::kBytes},
          value_{std::move(value)}
    {}

    Parameter(Date value)
        : type_{ParameterType::kDate},
          value_{value}
    {}

    Parameter(Time value)
        : type_{ParameterType::kTime},
          value_{value}
    {}

    Parameter(Timestamp value)
        : type_{ParameterType::kTimestamp},
          value_{value}
    {}

    template <std::size_t Precision, std::size_t Scale>
    Parameter(const Decimal<Precision, Scale>& value)
        : type_{ParameterType::kDecimal},
          value_{DecimalParameter{
              std::string{value.GetRepresentation()},
              static_cast<std::uint8_t>(Precision),
              static_cast<std::uint8_t>(Scale),
          }}
    {}

    template <typename T>
    requires kIsParameterStoreValue<std::optional<T>>
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
        using Value = std::remove_cv_t<T>;
        if constexpr (io::traits::kHasValidBoundType<Value> &&
                      (!io::traits::kIsDirectBoundType<Value> || std::is_enum_v<Value>))
        {
            return NullOf<io::traits::BoundType<Value>>();
        }
        Parameter result{Value{}};
        result.is_null_ = true;
        return result;
    }

    ParameterType type_;
    bool is_null_{false};
    Value value_;
};

using ParameterList = std::vector<Parameter>;
using ParameterRows = std::vector<ParameterList>;

template <typename T>
requires kIsParameterArgument<T>
constexpr std::size_t ParameterArgumentWidth() {
    using Value = std::remove_cvref_t<T>;
    if constexpr (kIsParameterStoreValue<Value>) {
        return 1;
    } else {
        return boost::pfr::tuple_size_v<Value>;
    }
}

template <typename T>
requires kIsParameterArgument<T>
void AppendParameterArgument(ParameterList& result, const T& argument) {
    using Value = std::remove_cvref_t<T>;
    if constexpr (kIsParameterStoreValue<Value>) {
        result.emplace_back(argument);
    } else {
        boost::pfr::for_each_field(argument, [&result](const auto& field) { result.emplace_back(field); });
    }
}

template <typename... Args>
requires((kIsParameterArgument<Args> && ...))
ParameterList MakeParameterList(const Args&... args) {
    ParameterList result;
    result.reserve((ParameterArgumentWidth<Args>() + ... + std::size_t{0}));
    (AppendParameterArgument(result, args), ...);
    return result;
}

}  // namespace storages::odbc::impl

USERVER_NAMESPACE_END
