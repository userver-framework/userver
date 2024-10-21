# uPg: Supported data types

PostgreSQL provides data type support with a system of buffer parsers and
formatters. Please refer to @ref pg_io for more information about the system.

@see @ref userver_postgres_parse_and_format


@anchor pg_user_types
## Fundamental PostgreSQL types

The fundamental PostgreSQL types support is provided by the driver. The
table below shows supported PostgreSQL types and their mapping to C++ types
provided by the driver. Column "Default" marks the PostgreSQL type to which
a C++ type is mapped when used as a parameter. Where the C++ type is N/A
it means that the PostgreSQL data type is not supported. When there is a
C++ type in parenthesis, it is a data type that will be supported later
and the C++ type is planned counterpart.
PG type           | C++ type                                | Default |
----------------- | --------------------------------------- | ------- |
smallint          | std::int16_t                            | +       |
integer           | std::int32_t                            | +       |
bigint            | std::int64_t                            | +       |
smallserial       | std::int16_t                            |         |
serial            | std::int32_t                            |         |
bigserial         | std::int64_t                            |         |
boolean           | bool                                    | +       |
real              | float                                   | +       |
double precision  | double                                  | +       |
numeric(p)        | decimal64::Decimal                      | +       |
decimal(p)        | decimal64::Decimal                      | +       |
money             | N/A                                     |         |
text              | std::string                             | +       |
char(n)           | std::string                             |         |
varchar(n)        | std::string                             |         |
"char"            | char                                    | +       |
timestamp         | storages::postgres::TimePointWithoutTz  | +       |
timestamptz       | storages::postgres::TimePointTz         | +       |
date              | utils::datetime::Date                   | +       |
time              | utils::datetime::TimeOfDay              | +       |
timetz            | N/A                                     |         |
interval          | std::chrono::microseconds               |         |
bytea             | container of one-byte type              |         |
bit(n)            | utils::Flags                            |         |
^                 | std::bitset<N>                          |         |
^                 | std::array<bool, N>                     |         |
bit varying(n)    | utils::Flags                            |         |
^                 | std::bitset<N>                          |         |
^                 | std::array<bool, N>                     |         |
uuid              | boost::uuids::uuid                      | +       |
json              | formats::json::Value                    |         |
jsonb             | formats::json::Value                    | +       |
int4range         | storages::postgres::IntegerRange        |         |
^                 | storages::postgres::BoundedIntegerRange |         |
int8range         | storages::postgres::BigintRange         |         |
^                 | storages::postgres::BoundedBigintRange  |         |
inet              | utils::ip::AddressV4                    |         |
^                 | utils::ip::AddressV6                    |         |
cidr              | utils::ip::NetworkV4                    |         |
^                 | utils::ip::NetworkV6                    |         |
macaddr           | utils::Macaddr                          |         |
macaddr8          | utils::Macaddr8                         |         |
numrange          | N/A                                     |         |
tsrange           | N/A                                     |         |
tstzrange         | N/A                                     |         |
daterange         | N/A                                     |         |

@warning The library doesn't provide support for C++ unsigned integral
types intentionally as PostgreSQL doesn't provide unsigned types and
using the types with the database is error-prone.


@anchor pg_timestamp
## Timestamp Support aka TimePointTz and TimePointWithoutTz

The driver provides mapping from C++ std::chrono::time_point template type to
Postgres timestamp (without time zone) data type.

To read/write timestamp with time zone Postgres data type a
storages::postgres::TimePointTz helper type is provided.

PostgreSQL internal timestamp resolution is microseconds.

**Example:**

@snippet postgresql/src/storages/postgres/tests/chrono_pgtest.cpp  tz sample

### How not to get skewed times in the PostgreSQL

Postgres has two types corresponding to absolute time
(a.k.a. global time; Unix time; NOT local time):
 * `TIMESTAMP WITH TIME ZONE`
 * `TIMESTAMP`

An unfortunate design decision on the PostgreSQL side is that it allows
**implicit conversion** between them, and database applies an offset to the
time point when doing so, depending on the timezone of the Postgres database.

Because of this you MUST ensure that you always use the correct type:
  * storages::postgres::TimePointTz for `TIMESTAMP WITH TIME ZONE`;
  * storages::postgres::TimePointWithoutTz for `TIMESTAMP`.

Otherwise, you'll get skewed times in database:

@snippet postgresql/src/storages/postgres/tests/chrono_pgtest.cpp  tz skewed

There is no way to detect that issue on the userver side, as the implicit
conversion is performed by the database itself and it provides no information
that the conversion happened.


@anchor pg_arrays
## Arrays in PostgreSQL

The driver supports PostgreSQL arrays provided that the element type is
supported by the driver, including user types.

Array parser will throw storages::postgres::DimensionMismatch if the
dimensions of C++ container do not match that of the buffer received from
the server.

Array formatter will throw storages::postgres::InvalidDimensions if
containers on same level of depth have different sizes.


## User-defined PostgreSQL types

The driver provides support for user-defined PostgreSQL types:
- domains
- enumerations
- composite types
- custom ranges

For more information please see
@ref scripts/docs/en/userver/pg_user_types.md.


## C++ strong typedefs in PostgreSQL

The driver provides support for C++ strong typedef idiom. For more
information see @ref pg_strong_typedef


## PostgreSQL ranges

PostgreSQL range type support is provided by `storages::postgres::Range`
template.


## Geometry types in PostgreSQL

For geometry types the driver provides parsing/formatting from/to
on-the-wire representation. The types provided do not define any calculus.


@anchor pg_bytea
## PostgreSQL bytea support

The driver allows reading and writing raw binary data from/to PostgreSQL
`bytea` type.

Reading and writing to PostgreSQL is implemented for `std::string`,
`std::string_view` and `std::vector` of `char` or `unsigned char`.

@warning When reading to `std::string_view` the value MUST NOT be used after
the PostgreSQL result set is destroyed.

Bytea() is a helper function for reading and writing binary data from/to a database.

Example usage of Bytea():
@snippet postgresql/src/storages/postgres/tests/bytea_pgtest.cpp bytea_simple
@snippet postgresql/src/storages/postgres/tests/bytea_pgtest.cpp bytea_string
@snippet postgresql/src/storages/postgres/tests/bytea_pgtest.cpp bytea_vector


## Network types in PostgreSQL

The driver offers data types to store IPv4, IPv6, and MAC addresses, as
well as network specifications (CIDR).


## Bit string types in PostgreSQL

The driver supports PostgreSQL `bit` and `bit varying` types.

Parsing and formatting is implemented for integral values
(e.g. `uint32_t`, `uint64_t`), `utils::Flags`, `std::array<bool, N>`
and `std::bitset<N>`.

Example of using the bit types from tests:
@snippet storages/postgres/tests/bitstring_pgtest.cpp Bit string sample


## PostgreSQL types not covered above

The types not covered above or marked as N/A in the table of fundamental
types will be eventually supported later, on request from the driver's
users.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref pg_process_results | @ref pg_user_row_types ⇨
@htmlonly </div> @endhtmlonly
