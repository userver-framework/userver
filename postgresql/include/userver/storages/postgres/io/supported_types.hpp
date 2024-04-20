#pragma once

/// @page pg_types uPg: Supported data types
///
/// uPg provides data type support with a system of buffer parsers and
/// formatters.
/// Please refer to @ref pg_io for more information about the system.
///
/// @see @ref userver_postgres_parse_and_format
///
/// @par Fundamental PostgreSQL types
///
/// The fundamental PostgreSQL types support is provided by the driver. The
/// table below shows supported Postgres types and their mapping to C++ types
/// provided by the driver. Column "Default" marks the Postgres type to which
/// a C++ type is mapped when used as a parameter. Where the C++ type is N/A
/// it means that the PosgreSQL data type is not supported. When there is a
/// C++ type in parenthesis, it is a data type that will be supported later
/// and the C++ type is planned counterpart.
/// PG type           | C++ type                                | Default |
/// ----------------- | --------------------------------------- | ------- |
/// smallint          | std::int16_t                            | +       |
/// integer           | std::int32_t                            | +       |
/// bigint            | std::int64_t                            | +       |
/// smallserial       | std::int16_t                            |         |
/// serial            | std::int32_t                            |         |
/// bigserial         | std::int64_t                            |         |
/// boolean           | bool                                    | +       |
/// real              | float                                   | +       |
/// double precision  | double                                  | +       |
/// numeric(p)        | decimal64::Decimal                      | +       |
/// decimal(p)        | decimal64::Decimal                      | +       |
/// money             | N/A                                     |         |
/// text              | std::string                             | +       |
/// char(n)           | std::string                             |         |
/// varchar(n)        | std::string                             |         |
/// "char"            | char                                    | +       |
/// timestamp         | std::chrono::system_clock::time_point   | +       |
/// timestamptz       | storages::postgres::TimePointTz         | +       |
/// date              | utils::datetime::Date                   | +       |
/// time              | utils::datetime::TimeOfDay              | +       |
/// timetz            | N/A                                     |         |
/// interval          | std::chrono::microseconds               |         |
/// bytea             | container of one-byte type              |         |
/// bit(n)            | utils::Flags                            |         |
/// ^                 | std::bitset<N>                          |         |
/// ^                 | std::array<bool, N>                     |         |
/// bit varying(n)    | utils::Flags                            |         |
/// ^                 | std::bitset<N>                          |         |
/// ^                 | std::array<bool, N>                     |         |
/// uuid              | boost::uuids::uuid                      | +       |
/// json              | formats::json::Value                    |         |
/// jsonb             | formats::json::Value                    | +       |
/// int4range         | storages::postgres::IntegerRange        |         |
/// ^                 | storages::postgres::BoundedIntegerRange |         |
/// int8range         | storages::postgres::BigintRange         |         |
/// ^                 | storages::postgres::BoundedBigintRange  |         |
/// inet              | utils::ip::AddressV4                    |         |
/// ^                 | utils::ip::AddressV6                    |         |
/// cidr              | utils::ip::NetworkV4                    |         |
/// ^                 | utils::ip::NetworkV6                    |         |
/// macaddr           | utils::Macaddr                          |         |
/// macaddr8          | utils::Macaddr8                         |         |
/// numrange          | N/A                                     |         |
/// tsrange           | N/A                                     |         |
/// tstzrange         | N/A                                     |         |
/// daterange         | N/A                                     |         |
///
/// @warning The library doesn't provide support for C++ unsigned integral
/// types intentionally as PostgreSQL doesn't provide unsigned types and
/// using the types with the database is error-prone.
///
/// For more information on timestamps and working with time zones please see
/// @ref pg_timestamp
///
/// @anchor pg_arrays
/// @par Arrays
///
/// The driver supports PostgreSQL arrays provided that the element type is
/// supported by the driver, including user types.
///
/// Array parser will throw storages::postgres::DimensionMismatch if the
/// dimensions of C++ container do not match that of the buffer received from
/// the server.
///
/// Array formatter will throw storages::postgres::InvalidDimensions if
/// containers on same level of depth have different sizes.
///
/// @par User-defined PostgreSQL types
///
/// The driver provides support for user-defined PostgreSQL types:
/// - domains
/// - enumerations
/// - composite types
/// - custom ranges
///
/// For more information please see
/// @ref scripts/docs/en/userver/pg_user_types.md.
///
/// @par C++ strong typedefs
///
/// The driver provides support for C++ strong typedef idiom. For more
/// information see @ref pg_strong_typedef
///
/// @par PostgreSQL ranges
///
/// PostgreSQL range type support is provided by `storages::postgres::Range`
/// template.
///
/// @par Geometry types
///
/// For geometry types the driver provides parsing/formatting from/to
/// on-the-wire representation. The types provided do not define any calculus.
///
/// @anchor pg_bytea
/// @par PostgreSQL bytea support
///
/// The driver allows reading and writing raw binary data from/to PostgreSQL
/// `bytea` type.
///
/// Reading and writing to PostgreSQL is implemented for `std::string`,
/// `std::string_view` and `std::vector` of `char` or `unsigned char`.
///
/// @warning When reading to `std::string_view` the value MUST NOT be used after
/// the PostgreSQL result set is destroyed.
///
/// @code{.cpp}
/// namespace pg = storages::postgres;
/// using namespace std::string_literals;
/// std::string s = "\0\xff\x0afoobar"s;
/// trx.Execute("select $1", pg::Bytea(tp));
/// @endcode
///
/// @par Network types
///
/// The driver offers data types to store IPv4, IPv6, and MAC addresses, as
/// well as network specifications (CIDR).
///
/// @par Bit string types
///
/// The driver supports PostgreSQL `bit` and `bit varying` types.
///
/// Parsing and formatting is implemented for integral values
/// (e.g. `uint32_t`, `uint64_t`), `utils::Flags`, `std::array<bool, N>`
/// and `std::bitset<N>`.
///
/// Example of using the bit types from tests:
/// @snippet storages/postgres/tests/bitstring_pgtest.cpp Bit string sample
///
/// @par PostgreSQL types not covered above
///
/// The types not covered above or marked as N/A in the table of fundamental
/// types will be eventually supported later, on request from the driver's
/// users.
///
/// ----------
///
/// @htmlonly <div class="bottom-nav"> @endhtmlonly
/// ⇦ @ref pg_process_results | @ref pg_user_row_types ⇨
/// @htmlonly </div> @endhtmlonly

//@{
/** @name Traits etc */
#include <userver/storages/postgres/io/nullable_traits.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
//@}

//@{
/** @name Data types */
#include <userver/storages/postgres/io/bytea.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/decimal64.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/storages/postgres/io/floating_point_types.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/string_types.hpp>
#include <userver/storages/postgres/io/time_of_day.hpp>
#include <userver/storages/postgres/io/uuid.hpp>
//@}

//@{
/** @name Row types */
#include <userver/storages/postgres/io/row_types.hpp>
//@}

//@{
/** @name Type derivatives */
#include <userver/storages/postgres/io/array_types.hpp>
#include <userver/storages/postgres/io/composite_types.hpp>
#include <userver/storages/postgres/io/optional.hpp>
#include <userver/storages/postgres/io/range_types.hpp>
#include <userver/storages/postgres/io/strong_typedef.hpp>
//@}

//@{
/** @name JSON types */
#include <userver/storages/postgres/io/json_types.hpp>
//@}

//@{
/** @name User type registry */
#include <userver/storages/postgres/io/user_types.hpp>
//@}
