# uPg: PostgreSQL user type mappings

This section describes advanced usage of PostgreSQL type system with custom
user-povided database types.

For a basic information on querying data see @ref pg_run_queries and
@ref pg_process_results. A list of supported fundamental PostgreSQL types
and their mappings to C++ types is available at @ref pg_types.

In PosgtgreSQL database the following kinds of user types are available:
  - @ref pg_composite_types "composite (row) types"
  - @ref pg_enum "enumerations"
  - @ref pg_range_types "ranges"
  - @ref pg_user_types "domains"


@anchor pg_user_types
## Mapping a C++ type to PostgreSQL domain user type

Domains are essentially some database data types with database constraints
applied to them. Domains map to their base data types' C++ counterparts.

For a PostgreSQL database domain defined as:

@snippet storages/postgres/tests/user_types_pgtest.cpp User domain type postgres

The following code allows retrieval of that type:

@snippet storages/postgres/tests/user_types_pgtest.cpp User domain type cpp usage



@anchor pg_composite_types
## Mapping a composite (row) user type

The driver supports user-defined PostgreSQL composite types. The C++
counterpart type must satisfy the same requirements as for the row types,
(@ref pg_user_row_types) and must provide a specialization of
storages::postgres::io::CppToUserPg template (@ref pg_user_types).

After a C++ type is defined, it must be mapped to its PostgreSQL
counterpart by specialising storages::postgres::io::CppToUserPg template for the
type. C++ types are mapped to PostgreSQL types by their name, so the
specialization for storages::postgres::io::CppToUserPg template must have a
`static constexpr` member of type DBTypeName named `postgres_name`.

Parsing a composite structure from PostgreSQL buffer will throw an error if
the number of fields in the postgres data is different from the number of
data members in target C++ type. This is the only sanity control for the
composite types. The driver doesn't check the data type oids, it's user's
responsibility to provide structures with compatible data members.

For a PostgreSQL database type defined as:

@snippet storages/postgres/tests/user_types_pgtest.cpp User composite type postgres

The following C++ type should be written:

@snippet storages/postgres/tests/user_types_pgtest.cpp User composite type cpp

@warning The type mapping specialization **must** be accessible at the
points where parsing/formatting of the C++ type is instantiated. The
header where the C++ type is declared is an appropriate place to do it.

Now it is possible to select the datatype directly into a C++ type:

@snippet storages/postgres/tests/user_types_pgtest.cpp User composite type cpp usage

A connection gets the data types' definitions after connect and uses the
definitions to map C++ types to PostgreSQL type oids.


@anchor pg_enum
## Mapping a C++ enum to PostgreSQL enum type

A C++ enumeration can be mapped to a PostgreSQL enum type by providing a
list of PostgreSQL literals mapped to the enumeration values via a
specialization of storages::postgres::io::CppToUserPg template.

For example, if we have a PostgreSQL enum type:

@snippet storages/postgres/tests/enums_pgtest.cpp User enum type postgres

and a C++ enumeration declared as follows:

@snippet storages/postgres/tests/enums_pgtest.cpp User enum type cpp

The difference from mapping a PosgreSQL user type is that for an
enumeration we need to provide a list of enumerators with corresponding
literals. Note that the utils::TrivialBiMap could be reused in different parts
of code.

@warning The type mapping specialization **must** be accessible at the
points where parsing/formatting of the C++ type is instantiated. The
header where the C++ type is declared is an appropriate place to do it.

After the above steps the enum is ready to be used:

@snippet storages/postgres/tests/enums_pgtest.cpp User enum type cpp usage

There is an alternative way to specialize the
storages::postgres::io::CppToUserPg template without using utils::TrivialBiMap.
This way is much less efficient, so it is deprecated.

@snippet storages/postgres/tests/enums_pgtest.cpp User enum type cpp2

In the above example the specialization of storages::postgres::io::CppToUserPg
derives from storages::postgres::io::EnumMappingBase for it to
provide type aliases for `EnumeratorList` and `EnumType`. `EnumType` is an alias
to the enumeration type for convenience of declaring pairs, as the
enumeration can have a long qualified name.


@anchor pg_range_types
## User Range types

PostgreSQL provides a facility to represent intervals of values. They can be
bounded (have both ends), e.g [0, 1], [2, 10), unbounded (at least one end
is infinity or absent), e.g. (-∞, +∞), and empty.

The range that can be unbounded is modelled by storages::postgres::Range
template, which provides means of constructing any possible combination of
interval ends. It is very versatile, but not very convenient in most cases.
storages::postgres::BoundedRange template is an utility that works only
with bounded ranges.

A user can define
[custom range types](https://www.postgresql.org/docs/current/rangetypes.html#RANGETYPES-DEFINING)
and they can be mapped to C++ counterparts, e.g.

@snippet storages/postgres/tests/user_types_pgtest.cpp User domainrange type postgres

and declare a mapping from C++ range to this type just as for any other user
type:

@snippet storages/postgres/tests/user_types_pgtest.cpp User domainrange type cpp

Please note that `my_type` must be comparable both in Postgres and in C++.

The type could be used in code in the following way:

@snippet storages/postgres/tests/user_types_pgtest.cpp User domainrange type usage


### Time Range and Other Widely Used Types

If you need a range of PostgreSQL `float` type or `time` type (actually any
type mapped to C++ type that is highly likely used by other developers),
**do not** specialize mapping for `Range<float>`, `Range<double>` or
`Range<TimeOfDay<seconds>>`. Declare a strong typedef (utils::StrongTypedef) for
your range or bounded range and map it to your postgres range type.

Here is an example for a range type defined in PostgreSQL as:

@snippet storages/postgres/tests/user_types_pgtest.cpp User timerange type postgres

Mapping of the above type to a strong typedef with name TimeOfDay could be done
in the following way:

@snippet storages/postgres/tests/user_types_pgtest.cpp User timerange type cpp

The actual usage:

@snippet storages/postgres/tests/user_types_pgtest.cpp User timerange type usage


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/pg_connlimit_mode_auto.md | @ref scripts/docs/en/userver/kafka.md ⇨
@htmlonly </div> @endhtmlonly


