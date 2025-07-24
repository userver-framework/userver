## SQLite Driver

**Quality:** @ref QUALITY_TIERS "Silver Tier".

🐙 **userver** provides access to SQLite databases servers via
components::SQLite. The uSQLite driver is asynchronous, and with it one can
write queries like this: [Sample SQLite usage](https://github.com/userver-framework/userver/tree/develop/samples/sqlite_service)

No macros, no meta-structs, no boilerplate, just your types used directly.

### Features
- Asynchronous API based on userver coroutines;
- Automatic connection pooling with smart routing;
- Prepared statements with variadic-template parameter binding;
- Full transaction & savepoint support (RAII);
- Read-only cursors;
- Compile-time-safe composite bindings, runtime-checked primitives;
- Straightforward result extraction into C++ types;
- Bi-directional mapping between C++ and native SQLite types;
- Built-in support for userver types: Decimal64, JSON, boost::uuid;
- Nullable wrappers for all types via std::optional<T>;
- Seamless integration with userver infrastructure: configuration, logging, metrics, etc.

@section sqlite_info More information
- For configuring see components::SQLite
- For client operations see storages::sqlite::Client
- For C++ <-> SQLite mapping see @ref scripts/docs/en/userver/sqlite/supported_types.md
- For typed extraction of statements results into C++ types see
storages::sqlite::ResultSet
- For high-level design and implementation details see @ref scripts/docs/en/userver/sqlite/design_and_details.md
- SQLite Docs: https://www.sqlite.org/docs.html


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/ydb.md | @ref scripts/docs/en/userver/sqlite/supported_types.md ⇨
@htmlonly </div> @endhtmlonly
