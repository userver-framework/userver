# uClickHouse: C++ <-> ClickHouse mappings

uClickHouse driver doesn't expose underlying ClickHouse types and only provides a
way to convert results to strongly-typed structs/containers
(see storages::clickhouse::ExecutionResult).

As naive mapping is ambiguous for some types, namely for chrono and string,
explicit mapping is required by the driver - explicit specialization of
`CppToClickhouse` template.

@section types Supported Clickhouse types:
- DateTime @ref storages::clickhouse::io::columns::DateTimeColumn
- DateTime64([3, 6, 9]) @ref storages::clickhouse::io::columns::DateTime64Column
- Int8 @ref storages::clickhouse::io::columns::Int8Column
- Int32 @ref storages::clickhouse::io::columns::Int32Column
- Int64 @ref storages::clickhouse::io::columns::Int64Column
- UInt8 @ref storages::clickhouse::io::columns::UInt8Column
- UInt16 @ref storages::clickhouse::io::columns::UInt16Column
- UInt32 @ref storages::clickhouse::io::columns::UInt32Column
- UInt64 @ref storages::clickhouse::io::columns::UInt64Column
- String @ref storages::clickhouse::io::columns::StringColumn
- UUID @ref storages::clickhouse::io::columns::UuidColumn
- Nullable @ref storages::clickhouse::io::columns::NullableColumn
- Float32 @ref storages::clickhouse::io::columns::Float32Column
- Float64 @ref storages::clickhouse::io::columns::Float64Column

## Example usage:

@snippet storages/tests/execute_chtest.cpp  Sample CppToClickhouse specialization
