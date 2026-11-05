# uPg: Supported data types

PostgreSQL provides data type support with a system of buffer parsers and
formatters. Please refer to @ref scripts/docs/en/userver/pg/user_types.md and
@ref scripts/docs/en/userver/pg/user_row_types.md for more information about the system.

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
^                 | formats::json::RawString                |         |
^                 | storages::postgres::PlainJson           | +       |
jsonb             | formats::json::Value                    | +       |
^                 | formats::json::RawString                |         |
^                 | any type with io::kStoreAsJson          |         |
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


@anchor pg_json
## JSON and JSONB in PostgreSQL

`formats::json::Value` is the default type for reading and writing JSON. Query
parameters bind as **jsonb** by default; use `storages::postgres::PlainJson`
to bind as **json**.

`formats::json::RawString` forwards JSON from the database without parsing or
re-serialization. Reading from both `json` and `jsonb` columns is supported.
Query parameters bind as **jsonb**. To store the value in a `json` column or
bind it as `json`, explicitly cast in SQL (e.g. `INSERT INTO t (data) VALUES ($1::json)`).

@snippet postgresql/src/storages/postgres/tests/json_types_pgtest.cpp json_raw_string_as_set_of


@anchor pg_struct_as_json
### Storing a C++ structure as JSON

A type that already has a `formats::json` mapping can be stored in a `json` or
`jsonb` column directly, without naming the column's shape anywhere in SQL.
Give the type the usual `Parse`/`Serialize` pair:

@snippet postgresql/src/storages/postgres/tests/struct_as_json_pgtest.cpp struct_as_json_declare

and then opt in:

@snippet postgresql/src/storages/postgres/tests/struct_as_json_pgtest.cpp struct_as_json_opt_in

After that the structure is a parameter and a result type like any other:

@snippet postgresql/src/storages/postgres/tests/struct_as_json_pgtest.cpp struct_as_json_round_trip

Parameters bind as **jsonb**, the same default `formats::json::Value` has, so no
cast is needed to write into a `jsonb` column; cast in SQL (`$1::json`) to bind
as `json`. Reading works from either. `std::optional<T>` maps as you would
expect, so a nullable column needs nothing extra.

The opt-in is deliberately a separate declaration rather than being inferred
from the presence of `Parse` and `Serialize`: a great many types have those and
are not meant to live in a column. Declaring it on a type with no json mapping
maps nothing — both halves are required.

@warning **A `jsonb` column has no schema, so the structure is the only schema
there is — and every row holds whatever the structure looked like when that row
was written.** `ALTER TABLE` cannot migrate a document, and nothing will tell
you that an older row no longer matches the current type: you find out when the
read throws, which is on production data and long after the deploy.

So **write `Parse` permissively**. Give every member a default rather than
demanding it, and let an unknown member be ignored:

@snippet postgresql/src/storages/postgres/tests/struct_as_json_pgtest.cpp struct_as_json_older_row

That is what makes each kind of change to the structure safe, or not:

| change to the structure | what happens to rows already written |
|---|---|
| **a member is added** | safe if `Parse` defaults it — those rows simply do not have the key |
| **a member is removed** | safe to read; the data stays in the documents until something rewrites them |
| **a member is renamed** | **silent data loss on read**: the old key is still in the row and the new `Parse` ignores it, so the value reads as its default. A rename is a backfill, not a rename. |
| **a member changes type** | throws on every row written under the old type, unless `Parse` accepts both |
| **the build is rolled back** | the older build must tolerate members it has never heard of, which permissive parsing already gives |

A strict `Parse` — `json["page_size"].As<int>()` with no default — throws
`formats::json::MemberMissingException` on any row written before that member
existed. The driver cannot soften that, because it is the type's own `Parse`
doing the refusing; the test beside the snippets above asserts both sides of
that comparison.

@note This is a convenience for a structure whose shape is genuinely the
application's business — settings, a captured payload, a denormalised blob. It
is not a way to avoid declaring columns: the database cannot check the contents,
index them without a deliberate expression or GIN index, or join on them, and a
query cannot see inside them without `jsonb` operators.

----------

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
@ref scripts/docs/en/userver/pg/user_types.md.


## C++ strong typedefs in PostgreSQL

The driver provides support for C++ strong typedef idiom. For more
information see @ref scripts/docs/en/userver/pg/strong_typedef.md


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
@snippet postgresql/src/storages/postgres/tests/bitstring_pgtest.cpp Bit string sample


## PostgreSQL types not covered above

The types not covered above or marked as N/A in the table of fundamental
types will be eventually supported later, on request from the driver's
users.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg/process_results.md | @ref scripts/docs/en/userver/pg/user_row_types.md ⇨
@htmlonly </div> @endhtmlonly
