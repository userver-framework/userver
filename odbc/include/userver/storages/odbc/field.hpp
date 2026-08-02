#pragma once

/// @file userver/storages/odbc/field.hpp
/// @brief @copybrief storages::odbc::Field

#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/io/type_mapping.hpp>
#include <userver/storages/odbc/odbc_fwd.hpp>
#include <userver/storages/odbc/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

/// @cond
namespace impl {

template <typename T>
struct IsOptional : std::false_type {};

template <typename T>
struct IsOptional<std::optional<T>> : std::true_type {
    using ValueType = T;
};

template <typename T>
inline constexpr bool kIsOptional = IsOptional<std::remove_cv_t<T>>::value;

template <typename T>
inline constexpr bool kIsFieldScalar =
    std::same_as<std::remove_cv_t<T>, bool> ||
    (std::integral<std::remove_cv_t<T>> && !std::same_as<std::remove_cv_t<T>, bool> &&
     sizeof(std::remove_cv_t<T>) <= sizeof(std::uint64_t)) ||
    std::same_as<std::remove_cv_t<T>, float> || std::same_as<std::remove_cv_t<T>, double> ||
    std::same_as<std::remove_cv_t<T>, std::string> || std::same_as<std::remove_cv_t<T>, Bytes> ||
    std::same_as<std::remove_cv_t<T>, Date> || std::same_as<std::remove_cv_t<T>, Time> ||
    std::same_as<std::remove_cv_t<T>, Timestamp> || kIsDecimal<std::remove_cv_t<T>>;

template <typename T>
struct IsFieldAsType
    : std::bool_constant<
          kIsFieldScalar<T> || (!kIsFieldScalar<T> && io::traits::kHasFromOdbc<std::remove_cvref_t<T>>)> {};

template <typename T>
struct IsFieldAsType<std::optional<T>>
    : std::bool_constant<
          !io::traits::kHasMappingDeclaration<std::optional<T>> &&
          (kIsFieldScalar<T> || (!kIsFieldScalar<T> && io::traits::kHasFromOdbc<T>))> {};

template <typename T>
inline constexpr bool kIsFieldAsType = IsFieldAsType<std::remove_cv_t<T>>::value;

}  // namespace impl
/// @endcond

/// @brief Single cell in an ODBC result set row
class Field {
public:
    using size_type = std::size_t;

    size_type RowIndex() const { return row_index_; }
    size_type FieldIndex() const { return field_index_; }

    bool IsNull() const;

    std::string GetString() const;
    int64_t GetInt64() const;
    int32_t GetInt32() const;
    double GetDouble() const;
    bool GetBool() const;

    /// Converts the field with strict SQL category, NULL and range checks.
    /// Use `As<std::optional<T>>()` to accept SQL NULL.
    template <typename T>
    T As() const;

protected:
    friend class Row;

    Field() = default;

    Field(detail::ResultWrapperPtr res, size_type row, size_type col)
        : res_{std::move(res)},
          row_index_{row},
          field_index_{col}
    {}

private:
    std::int64_t GetSignedIntegerForAs() const;
    std::uint64_t GetUnsignedIntegerForAs() const;
    double GetFloatingPointForAs() const;
    std::string GetStringForAs() const;
    bool GetBoolForAs() const;
    Bytes GetBytesForAs() const;
    Date GetDateForAs() const;
    Time GetTimeForAs() const;
    Timestamp GetTimestampForAs() const;
    std::string GetDecimalForAs(std::size_t precision, std::size_t scale) const;

    detail::ResultWrapperPtr res_;
    size_type row_index_{0};
    size_type field_index_{0};
};

template <typename T>
T Field::As() const {
    using Value = std::remove_cv_t<T>;
    static_assert(
        !impl::kIsOptional<Value> || !io::traits::kHasMappingDeclaration<Value>,
        "CppToOdbc<std::optional<T>> is not supported; map T and use std::optional<T> for SQL NULL"
    );
    static_assert(impl::kIsFieldAsType<Value>, "Unsupported ODBC Field::As<T>() type");

    if constexpr (impl::kIsOptional<Value>) {
        using Inner = typename impl::IsOptional<Value>::ValueType;
        if (IsNull()) {
            return std::nullopt;
        }
        return Value{As<Inner>()};
    } else if constexpr (!impl::kIsFieldScalar<Value> && io::traits::kHasFromOdbc<Value>) {
        using Mapping = io::CppToOdbc<Value>;
        using BoundType = typename Mapping::BoundType;
        return Mapping::FromOdbc(As<BoundType>());
    } else if constexpr (std::same_as<Value, bool>) {
        return GetBoolForAs();
    } else if constexpr (std::signed_integral<Value>) {
        const auto value = GetSignedIntegerForAs();
        if (value < static_cast<std::int64_t>(std::numeric_limits<Value>::lowest()) ||
            value > static_cast<std::int64_t>(std::numeric_limits<Value>::max()))
        {
            throw ResultSetError("ODBC integer field does not fit into the requested signed type");
        }
        return static_cast<Value>(value);
    } else if constexpr (std::unsigned_integral<Value>) {
        const auto value = GetUnsignedIntegerForAs();
        if (value > static_cast<std::uint64_t>(std::numeric_limits<Value>::max())) {
            throw ResultSetError("ODBC integer field does not fit into the requested unsigned type");
        }
        return static_cast<Value>(value);
    } else if constexpr (std::same_as<Value, float>) {
        const auto value = GetFloatingPointForAs();
        if (value < static_cast<double>(std::numeric_limits<float>::lowest()) ||
            value > static_cast<double>(std::numeric_limits<float>::max()))
        {
            throw ResultSetError("ODBC floating-point field does not fit into float");
        }
        return static_cast<float>(value);
    } else if constexpr (std::same_as<Value, double>) {
        return GetFloatingPointForAs();
    } else if constexpr (std::same_as<Value, std::string>) {
        return GetStringForAs();
    } else if constexpr (std::same_as<Value, Bytes>) {
        return GetBytesForAs();
    } else if constexpr (std::same_as<Value, Date>) {
        return GetDateForAs();
    } else if constexpr (std::same_as<Value, Time>) {
        return GetTimeForAs();
    } else if constexpr (std::same_as<Value, Timestamp>) {
        return GetTimestampForAs();
    } else if constexpr (impl::kIsDecimal<Value>) {
        return Value{GetDecimalForAs(Value::kPrecision, Value::kScale)};
    }
}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
