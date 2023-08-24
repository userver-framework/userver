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
/// numeric(p)        | decimal64::Decimal<P>                   | +       |
/// decimal(p)        | decimal64::Decimal<P>                   | +       |
/// money             | N/A                                     |         |
/// text              | std::string                             | +       |
/// char(n)           | std::string                             |         |
/// varchar(n)        | std::string                             |         |
/// "char"            | char                                    | +       |
/// timestamp         | std::chrono::system_clock::time_point   | +       |
/// timestamptz       | storages::postgres::TimePointTz         | +       |
/// date              | utils::datetime::Date                   | +       |
/// time              | utils::datetime::TimeOfDay<>            | +       |
/// timetz            | N/A                                     |         |
/// interval          | std::chrono::microseconds               |         |
/// bytea             | container of one-byte type              |         |
/// bit(n)            | N/A                                     |         |
/// bit varying(n)    | N/A                                     |         |
/// uuid              | boost::uuids::uuid                      | +       |
/// json              | formats::json::Value                    |         |
/// jsonb             | formats::json::Value                    | +       |
/// int4range         | storages::postgres::IntegerRange        |         |
/// ^                 | storages::postgres::BoundedIntegerRange |         |
/// int8range         | storages::postgres::BigintRange         |         |
/// ^                 | storages::postgres::BoundedBigintRange  |         |
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
/// @par Arrays
///
/// The driver supports PostgreSQL arrays provided that the element type is
/// supported by the driver. See @ref pg_arrays for more information.
///
/// @par User-defined PostgreSQL types
///
/// The driver provides support for user-defined PostgreSQL types:
/// - domains
/// - enumerations
/// - composite types
///
/// For more information please see @ref pg_user_types.
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
/// @par Bytea
///
/// See @pg_bytea
///
/// @par PostgreSQL types not covered above
///
/// The types not covered above or marked as N/A in the table of fundamental
/// types will be eventually supported later, on request from the driver's
/// users.
///
/// - Bit string https://st.yandex-team.ru/TAXICOMMON-374
/// - Network types https://st.yandex-team.ru/TAXICOMMON-377
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
