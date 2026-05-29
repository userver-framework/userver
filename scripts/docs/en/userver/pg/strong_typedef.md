# uPg: support for C++ 'strong typedef' idiom

Within userver a strong typedef can be expressed as an enum class for
integral types and as an instance of
`USERVER_NAMESPACE::utils::StrongTypedef` template for all types. Both of
them are supported transparently by the PostgresSQL driver with minor
limitations. No additional code required.

@warning The underlying integral type for a strong typedef MUST be
signed as PostgreSQL doesn't have unsigned types

The underlying type for a strong typedef must be mapped to a system or a
user PostgreSQL data type. Strong typedef type derives nullability from
underlying type.

Using `USERVER_NAMESPACE::utils::StrongTypedef` example:
@snippet storages/postgres/tests/strong_typedef_pgtest.cpp Strong typedef

Using `enum class` example:
@snippet storages/postgres/tests/strong_typedef_pgtest.cpp Enum typedef
