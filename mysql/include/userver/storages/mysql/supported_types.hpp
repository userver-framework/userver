#pragma once

/// @file userver/storages/mysql/supported_types.hpp
/// This file is mostly for documentation purposes.
/// It also contains some static_asserts, but no actual functionality.

#include <userver/storages/mysql/impl/io/binder_declarations.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::supported_types {

namespace impl {

template <typename T>
using InputBinder = decltype(mysql::impl::io::GetInputBinder(
    std::declval<mysql::impl::InputBindingsFwd&>(), std::declval<std::size_t>(),
    std::declval<const T&>()));

template <typename T>
using OutputBinder = decltype(mysql::impl::io::GetOutputBinder(
    std::declval<mysql::impl::OutputBindingsFwd&>(),
    std::declval<std::size_t>(), std::declval<T&>()));

template <typename T>
inline constexpr bool kIsTypeSupportedAsInput =
    InputBinder<T>::kIsSupported&& InputBinder<std::optional<T>>::kIsSupported;

template <typename T>
inline constexpr bool kIsTypeSupportedAsOutput = OutputBinder<T>::kIsSupported&&
    OutputBinder<std::optional<T>>::kIsSupported;
}  // namespace impl

template <typename T>
inline constexpr bool kIsTypeSupported =
    impl::kIsTypeSupportedAsInput<T>&& impl::kIsTypeSupportedAsOutput<T>;

// clang-format off
/// @page userver_mysql_types
/// The uMySQL driver supports various `std::` types for both input and output
/// directions and some userver-specific types. <br>
/// The driver also supports aggregates of supported types as input or output parameters:<br>
/// whenever there is a row-wise mapping to/from `T`, `T` is expected to be an SimpleAggregate,
/// that is an aggregate type without base classes, const fields, references, or C arrays.
///
///
/// The following table shows how types are mapped from C++ to MySQL types.
///
/// ## C++ to MySQL Mapping
/// Input C++ types                             |   Corresponding MySQLType
/// ------------------------------------------- | -------------------------
/// `std::uint8_t`/`std::int8_t`                | `TINYINT`
/// `std::uint16_t`/`std::int16_t`              | `SMALLINT`
/// `std::uint32_t`/`std::int32_t`              | `INT`
/// `std::uint64_t`/`std::int64_t`              | `BIGINT`
/// `float`                                     | `FLOAT`
/// `double`                                    | `DOUBLE`
/// `std::string`                               | `TEXT` // TODO : maybe blob?
/// `std::string_view`                          | `TEXT` // TODO : maybe blob?
/// `const char *`                              | `TEXT` // TODO : maybe blob?
/// `std::chrono::system_clock::timepoint`      | `TIMESTAMP`
/// `userver::storages::mysql::Date`            | `DATE`
/// `userver::storages::mysql::DateTime`        | `DATETIME`
/// `userver::formats::json::Value`             | `TEXT` // TODO : what is this?
/// `userver::decimal64::Decimal<Prec, Policy>` | `DECIMAL`
///
/// The driver also supports std::optional<T> for every supported type,
/// and that maps to either NULL or exactly as T does, depending on whether is
/// optional engaged or not.
///
///
/// Mapping MySQL types to C++ types is a more complicated matter:<br>
/// First, the driver forbids mapping signed to unsigned or vice versa - an
/// attempt to do so will abort in debug build and throw in release (this
/// behavior is referred as UINVARIANT later), this check happens before actual
/// extraction. <br>
/// Second, the driver forbids mapping NULLABLE column into
/// non-optional C++ type, an attempt to do so will UINVARIANT, this check
/// happens before actual extraction. However other combinations of
/// optional/non-optional<->NULL/NOT NULL are supported:
/// Is column NULLABLE | Is type optional | behavior
/// ----------------- | ----------------- | --------
/// `true`            | `true`            | allowed
/// `true`            | `false`           | `UINVARIANT`
/// `false`           | `true`            | allowed
/// `false`           | `false`           | allowed
///
/// Third, an attempt to parse `DECIMAL` will throw at extraction stage if precision/rounding policy
/// of corresponding `decimal::Decimal64<Prec, Policy>` is violated. <br>
/// Fourth, an attempt to parse `DATE` or `DATETIME` not within `TIMEPOINT`
/// range into `std::chrono::system_clock::timepoint` will throw at extraction stage.
/// The decision was made to allow this conversion, since it might be very convenient to use,
/// but in general this conversion is narrowing. <br>
/// Fourth, general type mismatch will `UINVARIANT`, however widening numeric
/// conversions are allowed, this check happens before actual extraction.
/// You can see to what C++ types MySQL types can map in the following table:
/// ## MySQL to C++ mapping
/// Output C++ type                                             | Allowed MySQL types
/// ----------------------------------------------------------- | -------------------
/// `std::uint8_t`                                              | `TINYINT UNSIGNED NOT NULL`
/// `std::int8_t`                                               | `TINYINT NOT NULL`
/// `std::optional<std::uint8_t>`                               | `TINYINT UNSIGNED NOT NULL`, `TINYINT UNSIGNED`
/// `std::optional<std::int8_t>`                                | `TINYINT NOT NULL`, `TINYINT`
/// `std::uint16_t`/`std::int16_t`/`optional`                   | `SMALLINT` + all types above, with respect to signed/unsigned and NULL/NOT NULL
/// `std::uint32_t`/`std::int32_t`/`optional`                   | `INT`, `INT24` + all types above, with respect to signed/unsigned and NULL/NOT NULL
/// `std::uint64_t`/`std::int64_t`/`optional`                   | `BIGINT` + all types above, with respect to signed/unsigned and NULL/NOT NULL
/// `float`                                                     | `FLOAT NOT NULL`
/// `std::optional<float>`                                      | `FLOAT NOT NULL`, `FLOAT`
/// `double`                                                    | `DOUBLE NOT NULL`, `FLOAT NOT NULL`
/// `std::optional<double>`                                     | `DOUBLE`, `DOUBLE NOT NULL`, `FLOAT`, `FLOAT NOT NULL`
/// `std::string/optional`                                      | `CHAR`, `BINARY`, `VARCHAR`, `VARBINARY`, `TINYBLOB`, `TINYTEXT`, `BLOB`, `TEXT`, `MEDIUMBLOB`, `MEDIUMTEXT`, `LONGBLOB`, `LONGTEXT`, with respect to NULL/NOT NULL
/// `std::chrono::system_clock::timepoint/optional`             | `TIMEPOINT`, `DATETIME`, `DATE`, with respect to NULL/NOT NULL
/// `userver::storages::mysql::Date`/optional                   | `DATE`, with respect to NULL/NOT NULL
/// `userver::storages::mysql::DateTime`/optional               | `DATETIME`, with respect to NULL/NOT NULL
/// `userver::formats::json::Value/optional`                    | `JSON`, with respect to NULL/NOT NULL
/// `userver::decimal64::Decimal<Prec, Policy>`/optional        | `DECIMAL`, with respect to NULL/NOT NULL
///
///
// clang-format on

// Various numeric types
static_assert(kIsTypeSupported<std::uint8_t>);
static_assert(kIsTypeSupported<std::int8_t>);
static_assert(kIsTypeSupported<std::uint16_t>);
static_assert(kIsTypeSupported<std::int16_t>);
static_assert(kIsTypeSupported<std::uint32_t>);
static_assert(kIsTypeSupported<std::int32_t>);
static_assert(kIsTypeSupported<std::uint64_t>);
static_assert(kIsTypeSupported<std::int64_t>);
static_assert(kIsTypeSupported<float>);
static_assert(kIsTypeSupported<double>);

// strings
static_assert(kIsTypeSupported<std::string>);
static_assert(impl::kIsTypeSupportedAsInput<std::string_view>);  // input only

// dates
static_assert(kIsTypeSupported<std::chrono::system_clock::time_point>);
static_assert(kIsTypeSupported<storages::mysql::Date>);
static_assert(kIsTypeSupported<storages::mysql::DateTime>);

// Json
static_assert(kIsTypeSupported<formats::json::Value>);

// Every valid specialization of decimal64 is supported, not just these
static_assert(kIsTypeSupported<decimal64::Decimal<1>>);
static_assert(kIsTypeSupported<decimal64::Decimal<4>>);
static_assert(
    kIsTypeSupported<decimal64::Decimal<9, decimal64::FloorRoundPolicy>>);
static_assert(
    kIsTypeSupported<decimal64::Decimal<7, decimal64::HalfUpRoundPolicy>>);

}  // namespace storages::mysql::supported_types

USERVER_NAMESPACE_END
