## MySQL supported types

The uMySQL driver supports various `std::` types for both input and output
directions and some userver-specific types. <br>
The driver also supports aggregates of supported types as input or output parameters:<br>
whenever there is a row-wise mapping to/from `T`, `T` is expected to be an SimpleAggregate,
that is an aggregate type without base classes, const fields, references, or C arrays.

The following table shows how types are mapped from C++ to MySQL types.
### C++ to MySQL Mapping
| Input C++ types                        | Corresponding MySQLType |
|----------------------------------------|-------------------------|
| `std::uint8_t`/`std::int8_t`           | `TINYINT`               |
| `std::uint16_t`/`std::int16_t`         | `SMALLINT`              |
| `std::uint32_t`/`std::int32_t`         | `INT`                   |
| `std::uint64_t`/`std::int64_t`         | `BIGINT`                |
| `float`                                | `FLOAT`                 |
| `double`                               | `DOUBLE`                |
| `std::string`                          | `TEXT`                  |
| `std::string_view`                     | `TEXT`                  |
| `const char *`                         | `TEXT`                  |
| `std::chrono::system_clock::timepoint` | `TIMESTAMP`             |
| storages::mysql::Date                  | `DATE`                  |
| storages::mysql::DateTime              | `DATETIME`              |
| formats::json::Value                   | `TEXT`                  |
| decimal64::Decimal<Prec, Policy>       | `DECIMAL`               |

The driver also supports std::optional<T> for every supported type,
and that maps to either NULL or exactly as T does, depending on whether is
optional engaged or not.

Mapping MySQL types to C++ types is a more complicated matter:
* First, the driver forbids mapping signed to unsigned or vice versa - an
attempt to do so will abort in debug build and throw in release (this
behavior is referred as UINVARIANT later), this check happens before actual
extraction. <br>
* Second, the driver forbids mapping NULLABLE column into
non-optional C++ type, an attempt to do so will UINVARIANT, this check
happens before actual extraction. However, other combinations of
optional/non-optional<->NULL/NOT NULL are supported:

| Is column NULLABLE | Is type optional | behavior     |
|--------------------|------------------|--------------|
| `true`             | `true`           | allowed      |
| `true`             | `false`          | `UINVARIANT` |
| `false`            | `true`           | allowed      |
| `false`            | `false`          | allowed      |

* Third, an attempt to parse `DECIMAL` will throw at extraction stage if precision/rounding policy
of corresponding `decimal::Decimal64<Prec, Policy>` is violated.
* Fourth, an attempt to parse `DATE` or `DATETIME` not within `TIMEPOINT`
range into `std::chrono::system_clock::timepoint` will throw at extraction stage.
The decision was made to allow this conversion, since it might be very convenient to use,
but in general this conversion is narrowing.
* Fifth, general type mismatch will `UINVARIANT`, however widening numeric
conversions are allowed, this check happens before actual extraction.
You can see to what C++ types MySQL types can map in the following table:

### MySQL to C++ mapping
| Output C++ type                                 | Allowed MySQL types                                                                                                                                                 |
|-------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------|
 | `std::uint8_t`                                  | `TINYINT UNSIGNED NOT NULL`                                                                                                                                         |
 | `std::int8_t`                                   | `TINYINT NOT NULL`                                                                                                                                                  |
 | `std::optional<std::uint8_t>`                   | `TINYINT UNSIGNED NOT NULL`, `TINYINT UNSIGNED`                                                                                                                     |
 | `std::optional<std::int8_t>`                    | `TINYINT NOT NULL`, `TINYINT`                                                                                                                                       |
 | `std::uint16_t`/`std::int16_t`/`optional`       | `SMALLINT` + all types above, with respect to signed/unsigned and NULL/NOT NULL                                                                                     |
 | `std::uint32_t`/`std::int32_t`/`optional`       | `INT`, `INT24` + all types above, with respect to signed/unsigned and NULL/NOT NULL                                                                                 |
 | `std::uint64_t`/`std::int64_t`/`optional`       | `BIGINT` + all types above, with respect to signed/unsigned and NULL/NOT NULL                                                                                       |
 | `float`                                         | `FLOAT NOT NULL`                                                                                                                                                    |
 | `std::optional<float>`                          | `FLOAT NOT NULL`, `FLOAT`                                                                                                                                           |
 | `double`                                        | `DOUBLE NOT NULL`, `FLOAT NOT NULL`                                                                                                                                 |
 | `std::optional<double>`                         | `DOUBLE`, `DOUBLE NOT NULL`, `FLOAT`, `FLOAT NOT NULL`                                                                                                              |
 | `std::string/optional`                          | `CHAR`, `BINARY`, `VARCHAR`, `VARBINARY`, `TINYBLOB`, `TINYTEXT`, `BLOB`, `TEXT`, `MEDIUMBLOB`, `MEDIUMTEXT`, `LONGBLOB`, `LONGTEXT`, with respect to NULL/NOT NULL |
 | `std::chrono::system_clock::timepoint/optional` | `TIMEPOINT`, `DATETIME`, `DATE`, with respect to NULL/NOT NULL                                                                                                      |
 | storages::mysql::Date + optional                | `DATE`, with respect to NULL/NOT NULL                                                                                                                               |
 | storages::mysql::DateTime +optional             | `DATETIME`, with respect to NULL/NOT NULL                                                                                                                           |
 | formats::json::Value + optional                 | `JSON`, with respect to NULL/NOT NULL                                                                                                                               |
 | decimal64::Decimal<Prec, Policy> + optional     | `DECIMAL`, with respect to NULL/NOT NULL                                                                                                                            |
