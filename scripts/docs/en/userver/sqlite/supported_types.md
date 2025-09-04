## SQLite supported types

SQLite stores values according to their dynamic type, not the column’s declared type: every cell carries its own storage class at runtime.
There are just five such classes — NULL, INTEGER, REAL, TEXT, and BLOB — and every declared column acquires an affinity that tells the engine which class it prefers when values are inserted.
When you bind parameters or read results with these storage classes are transparently mapped to familiar C++ types. Because of this flexible model the same column can hold mixed types, but the driver still enforces strong compile-time and run-time checks to keep your code type-safe.

The uSQLite driver supports various `std::` types for both input and output
directions and some userver-specific types. <br>
The driver also supports aggregates of supported types as input or output parameters:<br>
whenever there is a row-wise mapping to/from `T`, `T` is expected to be an SimpleAggregate,
that is an aggregate type without base classes, const fields, references, or C arrays.

The following table shows how types are mapped from C++ to SQLite types. Note that the table below represents only the closest canonical mapping; the driver itself imposes no additional constraints, so you are free to perform any type conversions that SQLite allows.
### C++ to SQLite Mapping
| Input C++ types                          | Corresponding SQLite Data Storage    |
|------------------------------------------|--------------------------------------|
| `std::uint8_t`/`std::int8_t`             | `INTEGER`                            |
| `std::uint16_t`/`std::int16_t`           | `INTEGER`                            |
| `std::uint32_t`/`std::int32_t`           | `INTEGER`                            |
| `std::uint64_t`/`std::int64_t`           | `INTEGER`                            |
| `float`                                  | `REAL`                               |
| `double`                                 | `REAL`                               |
| `std::string`                            | `TEXT`                               |
| `std::string_view`                       | `TEXT`                               |
| `vector<std::uint8_t>`                   | `BLOB`                               |
| `bool`                                   | `INTEGER`                            |
| `std::chrono::system_clock::time_point`  | `INTEGER`                            |
| `boost::uuids::uuid`                     | `BLOB`                               |
| `formats::json::Value`                   | `TEXT`                               |
| `decimal64::Decimal<Prec, Policy>`       | `TEXT`                               |

The driver also supports std::optional<T> to represent nullable fields. When binding parameters or retrieving results, std::nullopt corresponds to SQL NULL.

SQLite remains free to return any storage class for any column, and the driver will not curtail that flexibility. Still, it is wise to keep explicit track of the types you expect—mismatched semantics can surface as runtime errors. The table below therefore shows the closest, canonical mapping from each SQLite storage class to its recommended C++ counterpart.
### SQLite to C++ mapping
| Output C++ type                                    | Allowed SQLite types                   |
|----------------------------------------------------|----------------------------------------|
| `std::uint8_t`/`std::int8_t`/`optional`            | `INTEGER`                              |
| `std::uint16_t`/`std::int16_t`/`optional`          | `INTEGER`                              |
| `std::uint32_t`/`std::int32_t`/`optional`          | `INTEGER`                              |
| `std::uint64_t`/`std::int64_t`/`optional`          | `INTEGER`                              |
| `bool`                                             | `INTEGER`                              |
| `float`/`double`/`optional`                        | `REAL`                                 |
| `std::string/optional`                             | `TEXT`                                 |
| `std::chrono::system_clock::time_point`/`optional` | `TEXT`                                 |
| `formats::json::Value`/`optional`                  | `TEXT`                                 |
| `decimal64::Decimal<Prec, Policy>`/`optional`      | `TEXT`                                 |
| `vector<std::uint8_t>`                             | `BLOB`                                 |
| `boost::uuids::uuid`                               | `BLOB`                                 |


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/sqlite/sqlite_driver.md | @ref scripts/docs/en/userver/sqlite/design_and_details.md ⇨
@htmlonly </div> @endhtmlonly
