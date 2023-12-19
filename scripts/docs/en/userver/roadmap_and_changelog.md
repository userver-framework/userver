# Roadmap and Changelog

We keep an eye on all the issues and feature requests at the
[github.com/userver-framework](https://github.com/userver-framework), at the
[English-speaking](https://t.me/userver_en) and
[Russian-speaking](https://t.me/userver_ru) Telegram support chats. All the
good ideas are discussed, big and important ones go to the Roadmap. We also
have our in-house feature requests, those could be also found in Roadmap.

Important or interesting features go to the changelog as they get implemented.
Note that there's also a @ref scripts/docs/en/userver/security_changelog.md.

Changelog news also go to the
[userver framework news channel](https://t.me/userver_news).


## Roadmap

* Add YDB driver.
* Add PostgreSQL connection pools control via Congestion Control.
* Add retry budget or retry circuit breaker for clients.
* Add web interface to the [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf)
* Add basic Kafka driver.


## Changelog


### November 2023

* formats::json::Schema and validation function for it formats::json::Validate
  were implemented. Many thanks to [Aleksej Kamenev](https://github.com/ishiku) for
  the PR!
* PostgreSQL driver now auto detects the max pool size, see
  @ref scripts/docs/en/userver/pg_connlimit_mode_auto.md
* server::http::HttpRequest:GetMethod() now can return
  server::http::HttpMethod::kHEAD. Thanks
  to [Kirill Zimnikov](https://github.com/lirik90) for the bugreport and PR!
* PostgreSQL now has the following static config options:
  * `check-user-types` option to prevent service from starting if
    database is not ready for work (for example: some migrations were
    not applied).
  * `discard-all-on-connect` - to force running `DISCARD ALL` on new
    connections, which could be useful for some PostgreSQLs smart-proxies
    that reuse the same connections. 
* Boost.PFR 2.2.0 is now in use. The door for compile time reflection pull
  requests is now open!
* More metrics and fallbacks for logging errors.
* More tests for utils::datetime::MockSteadyNow. Thanks to
  [Aleksej Kamenev](https://github.com/ishiku) for the PR!
* components::CachingComponentBase now has a static config option
  `alert-on-failing-to-update-times` to fire an alert if the cache update failed
  specified amount of times in a row.
* Optimizations:
  * Caching of `dl_phdr_info` items is not ON by default, leading up to
    multiple seconds faster exception handling under heavy load when parts of
    the executable still being on hard drive rather than in memory.
    Use `USERVER_DISABLE_PHDR_CACHE` CMake option to disable it, if the
    framework reports `dlopen` usage after component start and there's no way
    to avoid it.
  * Optimized discarding logs by log level from 4ns to 2ns. Added CMake option
    `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` to totally eliminate all the CPU
    and binary size overhead from logging.
  * Implemented utils::SmallString::resize_and_overwrite() function as in C++23
    std::string. Thanks to [Илья Оплачкин](https://github.com/IoplachkinI) for
    the PR.
  * Up to an order of magnitude faster generation of HTTP response header via
    utils::SmallString usage. Thanks to
    [Илья Оплачкин](https://github.com/IoplachkinI) for the PR.
  * Faster logging of durations in tracing::Span, tracing::ScopeTime and
    friends.
  * Parsing inside storages::postgres::ResultSet now does not compute
    parsing-failure diagnostic information if parsing is successful.
  * Return PostgreSQL connection to pool earlier, before updating the
    statistics.
  * fs::RewriteFileContents() now does not `fsync` directories, dumping caches
    now also do not `fsync` directories leading to better performance while
    still properly restoring after server power-off. 
  * PostgreSQL typed parsing was optimized to not copy std::shared_ptrs.
  * TESTPOINT() and TESTPOINT_CALLBACK() now produce less instructions and
    guaranteed to not throw it the testpoints are disabled.
* Documentation:
  * @ref scripts/docs/en/userver/grpc.md now has a deeper explanation of
    middlewares
  * New @ref scripts/docs/en/userver/dynamic_config.md page and related samples.
  * Samples were significantly simplified, more static configuration options
    now have good defaults and do not require explicit setup.
  * @ref scripts/docs/en/userver/tutorial/build.md now contains information on
    how to build service templates. Information on how to build the framework
    tests was moved to scripts/docs/en/userver/tutorial/build_userver.md
  * Documented the server::handlers::ImplicitOptions.


### October 2023

* Added from-code alerts via alerts::Storage client and alerts::StorageComponent
  component.
* Added utils::statistics::Histogram.
* HTTP client now uses RATE metrics were possible.
* tracing::DefaultTracingManagerLocator now has `incoming-format`
  and `new-requests-format` static config options to configure the tracing
  headers to receive and send.
* Clickhouse now supports Array(T) type
* Added `max-ttl-sec` to the @ref POSTGRES_CONNECTION_SETTINGS dynamic config to
  force connections reopening.
* RabbitMQ now provides a Get() command to retrieve a single message. Thanks to
  [Vladislav Nepogodin](https://github.com/vnepogodin) for the PR!
* Optimizations:
  * Coroutines working set was reduced, leading to significantly less memory
    usage under most of the loads that do not use all the coroutines from pool
    at once.
  * ugrpc::server::ServerComponent now provides a `completion-queue-count` to
    scale better under heavy load.
  * Primary address string is now cached in HTTP server connection, leading to
    minor performance improvement.
  * cache::NWayLRU now works faster due to improved hashing logic.
  * Internal per-task timer size was reduced by 176 bytes.
  * Removed rcu::Variable usage on hot path of tracing::Span destruction,
    optimized writing of Opentracing files.
* Dynamic configs were changed:
    * dynamic_config::Key now requires config defaults at construction.
    * `dynamic-config-fallbacks` component was merged into `dynamic-config`,
      see components::DynamicConfig for more info.
    * utils::DaemonMain now has `--print-dynamic-config-defaults` program option
      that prints all the in-code defaults of the dynamic configs.
    * In-code defaults now could be overridden in static config of
      components::DynamicConfig via `defaults` option.
    * `USERVER_CHECK_AUTH_IN_HANDLERS` was removed.
* Documentation:
    * Documentation version switch was added to the bottom of the page.
    * gRPC SSL server credentials setup info was added into
      @ref scripts/docs/en/userver/grpc.md
    * Clarified behavior of server::http::HttpRequest::GetArg()
    * Added @ref scripts/docs/en/userver/tutorial/multipart_service.md
    * More clarifications for the
      @ref scripts/docs/en/userver/component_system.md.
    * @see @ref scripts/docs/en/userver/periodics.md now gives hints on
      tracing::Span usage.
    * More info on cancellations and different
      @ref scripts/docs/en/userver/synchronization.md
    * Simplified samples.
* Build:
  * Multiple build fixes and improvements for gRPC, including gRPC download via
    [CPM](https://github.com/cpm-cmake/CPM.cmake).
  * Conan was updated, thanks to [Anton](https://github.com/Jihadist)
    for the PR.

### Release v1.0.0

Big new features since the Beta announcement:

* Implemented WebSockets server
* Added MySQL driver
* RabbitMQ drived was added
* Implemented TLS server
* Enabled PostgreSQL pipelining
* Implemented and enabled Deadline Propagation
* Improved experience with metrics. Added Prometheus and Graphite metric
  formats. Provided a modern simple interface to write and test metrics.
* Added chaos tests for drivers
* Implemented streaming API for the HTTP
* Improved documentation, added more samples and descriptions, improved search.
* Numerous optimizations.
* Numerous build improvements, including Conan and Docker support.

Optimized and improved features that were available at the Beta announcement:

* gRPC client and server
* Mongo driver
* Redis driver
* PostgreSQL
* HTTP server and client
* Logging and Tracing
* ... and many other features.

Detailed descriptions could be found below.


### Beta (September 2023)

* WebSockets server and TLS server are now implemented as part of the
  @ref scripts/docs/en/userver/http_server.md "HTTP server"

* PostgreSQL pipelining is now implemented and turned on by default, leading to
  improved response times.

* @ref scripts/docs/en/userver/mongodb.md "Mongo Congestion Control"
  is implemented and turned on by default, leading to better stability of the
  service in case of high load on database.

* Initial logger is now initialized from the component config, leading to
  a more simple code and setup. The `--initial-logger` option now does nothing
  and will be removed soon.

* Added a @ref scripts/docs/en/userver/functional_testing.md "`userver_testsuite_add_simple()`"
  CMake function to simplify testsuite CMake configuration.

* Expanded list of HTTP codes, thanks to [Vladimir Popov](https://github.com/Liteskarr)
  for the PR!

* Projects from [Yandex Schools](https://academy.yandex.ru/schools) were updated
  by the original authors. Thanks to 
  [bshirokov](https://github.com/bshirokov),
  [Almaz Shagiev](https://github.com/bashkirian),
  [Konstantin Antoniadi](https://github.com/KonstantinAntoniadi),
  [Mingaripov Niyaz](https://github.com/mnink275),
  [Ilya Golosov](https://github.com/bookWorm21) and all the participants
  for the the great work and PRs!

* Build:
  * New versions of `yaml-cpp` library are now supported. Thanks to
    [Nikita](https://github.com/rtkid-nik) for the bug report!
  * Supported compilation with fmt 10.1.0. Thanks to
    [Vladislav Nepogodin](https://github.com/vnepogodin) for the PRs!
  * Fixed unused result warning. Thanks to
    [Vladislav Nepogodin](https://github.com/vnepogodin) for the PR!
  * Fixed use of deprecated API. Thanks to
    [Vladislav Nepogodin](https://github.com/vnepogodin) for the PR!
  * Fixed build with GCC-13 libstdc++. Thanks to
    [Vladislav Nepogodin](https://github.com/vnepogodin) for the PR!
  * Fixed MacOS Protobuf discovery. Thanks to
    [Pavel Talashchenko](https://github.com/pavelbezpravel) for the PR!
  * Fixed multiple other build warnings and issues.


### Beta (August 2023)

* Deadline Propagation is now implemented and
  @ref scripts/docs/en/userver/deadline_propagation.md "documented".

* Projects from [Yandex Schools](https://academy.yandex.ru/schools):
  * Documentation was redesigned by [hellenisaeva](https://github.com/hellenisaeva)
    and [MariaGrinchenko](https://github.com/MariaGrinchenko); new design was
    made up by [Fedor Alekseev](https://github.com/atlz253),
    [fleshr](https://github.com/fleshr),
    [Anna Zagrebaylo](https://github.com/ZenkoWu),
    [Michael Talalaev](https://github.com/InfinityScripter); the whole process
    was managed by [Oleg Komarov](https://github.com/0GE1) with feedback from
    marketing specialist [makachusha](https://github.com/makachusha).
    Many thanks to all the participants for the great work.
  * Implemented initial API for RFC 7616 - HTTP Digest Access Authentication,
    thanks to [Almaz Shagiev](https://github.com/bashkirian),
    [Konstantin Antoniadi](https://github.com/KonstantinAntoniadi),
    [Ilya Golosov](https://github.com/bookWorm21), and
    [Niyaz](https://github.com/mnink275) for multiple PRs!
  * std::bitset<N>, std::array<bool, N>, userver::utils::Flags<Enum> and integral
    values are now mapped to PostgreSQL bit and bit varying types. Thanks to
    [dsaa27](https://github.com/dsaa27) and 
    [bshirokov](https://github.com/bshirokov) for the PRs!
  * Initial `Realmedium` sample was implemented and moved into a separate
    repository at [userver-framework/realmedium_sample](https://github.com/userver-framework/realmedium_sample).
    Thanks to [berholz](https://github.com/berholz),
    [rumxcola](https://github.com/rumxcola),
    [Konstantin Artemev](https://github.com/rumxcola),
    [GasikPasik](https://github.com/GasikPasik),
    [Anna Volkova](https://github.com/etoanita),
    [Nikita Semenov](https://github.com/NikitaSemenov1),
    [Daniil Boriskin](https://github.com/Riferus),
    [VVendor](https://github.com/VVendor) for the PR!

* Metrics for CPU usage of particular threads of task processors are now
  available via new engine::TaskProcessorsLoadMonitor component.

* gRPC and HTTP handler metrics were switched to utils::statistics::Rate,
  leading to better handling of service start/stop by metrics storage.

* server::Server now returns HTTP header
  "Content-Type: application/octet-stream" if the content type was not set.

* Implemented `ToString(utils::StrongTypedef<floating point>)`, thanks to
  [Daniil Shvalov](https://github.com/danilshvalov) for the PR.

* Optimizations:
  * Signifiacally optimized JSON DOM access to a non-existing element.
  * Logging now does not lock a mutex in fs-task-processor on each log record.
  * Up to 3 times faster logging due to multiple minor tweaks with data copy and
    memory prereserving.
  * The new asynchronous engine::io::sys::linux::Inotify driver for Linux allows
    subscriptions to filesystem events. Thanks to it the fs::FsCacheClient now
    reacts faster.

* Docs:
  * @ref scripts/docs/en/userver/tutorial/auth_postgres.md now provides a
    PostgreSQL migrations sample.
  * Fixed broken refs to hello_service page, thanks to
    [Danil Sidoruk](https://github.com/eoan-ermine) for the PR!
  * @ref userver_universal group was added with information on contents of
    `userver-universal` CMake target.
  * Documented @ref scripts/docs/en/userver/service_monitor.md "PostgreSQL metrics"

* Build:
  * Dropped spdlog dependency
  * [CPM library](https://github.com/cpm-cmake/CPM.cmake) is now used for managing
    third-party dependencies.
  * Fixed cryptopp compilation error, thanks to
    [Daniil Shvalov](https://github.com/danilshvalov) for the PR!
  * Fixed -Wsign-conversion -Wctad-maybe-unsupported warnings in
    utils::TrivialBiMap, thanks to [darktime78](https://github.com/darktime78)
    for the PR!


### Beta (July 2023)

* server::http::CustomHandlerException now allows to provide a custom HTTP
  status that is not mapped to protocol-agnostic
  server::handlers::HandlerErrorCode.
* Non-coroutine `userver-universal` CMake target was refactored and is now used
  by the whole framework as a basic dependency. Compile times dropped down
  drastically for building the whole framework from scratch.
* Added a non-coroutine usage example @ref scripts/docs/en/userver/tutorial/json_to_yaml.md.
* It is now possible to subscribe to `SIGUSR1` and `SIGUSR2` in the same class.
  Thanks to [Beshkent](https://github.com/Beshkent) for the bug report.
* Dynamic config management commands were added to `uctl` tool.

* Optimizations:
  * storages::redis::SubscribeClient subscriptions were optimized to do less
    dynamic allocations on new message and use a faster lock-free queue.
  * Escaping of tags for logging is now done at compile-time by default, which
    gives up to 30% speedup in some cases.
  * Internal asynchronous logging logic was rewritten, leading to better
    scalability and more than 25% speedup.
  * HTTP client connection is now preserved on deadline, leading to less
    connections being reopened.

* Cleaned up and added docs for redis::CommandControl.
* MacOS build instructions were enhanced, thanks to
  [Daniil Shvalov](https://github.com/danilshvalov) for the PR!
* Conan build was fixed, thanks to
  [Yuri Bogomolov](https://github.com/ybogo) for the PR.
* Numerous build and configure fixes.


### Beta (June 2023)

* Static configs of the service now can retrieve environment variables via the
  `#env` syntax. See yaml_config::YamlConfig for more examples. 
* New testsuite plugins userver_config_http_client and
  userver_config_testsuite_support turned on by default to increase timeouts
  in tests and make the functional tests more reliable.
* gRPC now can write access logs, see `access-tskv-logger` static option of
  ugrpc::server::ServerComponent.
* Now the waiter is allowed to destroy the engine::SingleConsumerEvent
  immediately after exiting WaitForEvent, if the wait succeeded.
* New dynamic_config_fallback_patch fixture could be used to replace some
  dynamic config values specifically for testsuite tests.
* Fixed utils::SmallString tests, thanks to
  [Chingiz Sabdenov](https://github.com/GestaltEngine) for the PR!

* Optimizations:
  * Counters in the tasks were optimized. New thread local counters use
    less CPU and scale better on huge amounts of coroutines.
  * engine::TaskProcessor optimized layout with interference shielded atomics
    uses less CPU and scales better on huge amounts of coroutines.
  * Minor optimizations for HTTP clients URL manipulations.
  * tracing::Span generation of ID was made faster.
  * clients::http::Request now does less atomic counters increments/decrements
    while the request is being built.
  * `mlock_debug_info` static configuration option of
    components::ManagerControllerComponent is now on by default. It improves
    responsiveness of the service under heavy load on low memory and bad
    hard drives.
  * engine::io::Socket now has an additional `SendAll` overload that accepts
    `const struct iovec* list, std::size_t list_size` for implementing
    low-level vector sends. Mongo driver now uses the new function, resulting in
    smaller CPU and memory consumption.
  * clients::http::Form::AddContent now instead of `const std::string&` 
    parameters accepts `std::string_view` parameters that allow to copy less
    data.

* Docs and diagnostics:
  * gRPC metrics were documented.
  * New documentation page @ref scripts/docs/en/userver/http_server.md.
  * More debug logs for the Mongo error, thanks to
    [fominartem0](https://github.com/fominartem0) for the PR.
  * Improved diagnostics for CurrentSpan() and testsuite tasks.
  * Multiple typos fixed at @ref scripts/docs/en/userver/framework_comparison.md page, thanks
    to [Michael](https://github.com/technothecow) for the PR.

* Build:
  * Multiple build fixes for gRPC targets.
  * Clickhouse for Conan was added, thanks to [Anton](https://github.com/Jihadist)
    for the PR.
  * CMake build flag `USERVER_FEATURE_UBOOST_CORO` can be used to use system
    boost::context.


### Beta (May 2023)

* New scripts/uctl/uctl console script for administration of the running service
  was added.
* Improved compile times by removing multiple includes from userver headers
  including templating the serializers of different formats.
* Implemented ugrpc::server::HealthComponent handler, a gRPC alternative to
  server::handlers::Ping.
* gRPC server and clients now support middlewares - a customization plugins that
  could be shared by different handlers.
* Invalid implementations of CacheUpdateTrait::Update are now detected and
  logged.
  
* Optimizations:
  * Significant improvements in HTTP handling due to
    new http::headers::HeaderMap usage instead of std::unordered_map.
  * Improved performance of utils::TrivialBiMap by an order of magnitude
    for enum-to-enum mappings,
    thanks to [Vlad Tytskiy](https://github.com/Tytskiy) for the bug report!
  * utils::FromString now uses std::from_chars for better performance.
  * TESTPOINT and other testpoint related macro now imply zero overhead if
    testpoints were disabled in static config.
  * More functions of formats::json::ValueBuilder now accept std::string_view,
    resulting in less std::string constructions and better performance.
  * TSKV escaping was optimized via SIMD, resulting in up to x10 speedup on
    long logs.
  * `mlock_debug_info` static configuration option of
    components::ManagerControllerComponent now allows to lock exception
    unwinding information in memory. It improves responsiveness of the
    service under heavy load on low memory and bad hard drives.

* Docs:
  * Some metrics were documented, a human-readable format of metrics
    is now used in documentation. See @ref scripts/docs/en/userver/service_monitor.md.
  * Custom @ref scripts/docs/en/userver/.md404 "404 page".
  * New pages, including @ref scripts/docs/en/userver/faq.md, @ref scripts/docs/en/userver/periodics.md
    and @ref scripts/docs/en/userver/deploy_env.md.

* Build:
  * Arch Linux instructions were improved, thanks to
    [Kirill Zimnikov](https://github.com/lirik90) for the PR!
  * Fixed Conan based builds, thanks to [Anton](https://github.com/Jihadist)
    for the PR.
  * Clickhouse-cpp version was raised to 2.4.0, thanks to
    [Kirill Zimnikov](https://github.com/lirik90) for the PR!
  * Fixed build on libstdc++ from GCC-13, thanks to
    [Kirill Zimnikov](https://github.com/lirik90) for the PR!
  * Fixed benchmarks build on non x86 targets.
  * Rewrite of Protobuf and gRPC locating logic.


### Beta (April 2023)

* MySQL driver was added, see @ref mysql_driver.
* Experimental support for HTTP "Baggage" header is implemented, including
  verification, forwarding from HTTP handlers to client, baggage manipulation.
  See baggage::BaggageManagerComponent for more info.
* Redis driver now supports non-queued variants for pubsub.
* Redis driver now supports read-only transactions.
* utils::FilterBloom was merged in along with initial SLRU cache implementations.
  The work is a part of the
  [backend development school](https://academy.yandex.ru/schools/backend)
  course project by [Leonid Faktorovich](https://github.com/LeonidFaktorovich),
  [Alexandr Starovoytov](https://github.com/stewkk),
  [Ruslan](https://github.com/raslboyy), Egor Chistyakov from
  [PR #262](https://github.com/userver-framework/userver/pull/262).
* HTTP request decompression is now ON by default in
  server::handlers::HandlerBase.
* dynamic_config::Source now allows subscribing to a particular dynamic config
  variable changes.
* Initial support for utils::statistics::Rate metrics type.
* Human-readable "pretty" format (utils::statistics::ToPrettyFormat) for metrics
  output was added to the server::handlers::ServerMonitor.
* Optimizations:
  * Don't issue tail `writev` for empty `io_vec` on bulk socket writes.
  * All the userver metrics are now written via the fast
    utils::statistics::Writer.
  * x2-x50 faster serialization of unique maps into formats::json::ValueBuilder.
  * utils::StrIcaseHash became slightly faster.
  * engine::Task now does not have a virtual destructor. New engine::TaskBase
    based hierarchy does not use RTTI, resulting in smaller binaries.
  * Mongo driver does not capture stack traces in release builds in case of
    errors. The error path become slightly faster, server is more responsive
    in case of Mongo problems.
* Build:
  * Improved support for Conan 2.0, many thanks to
    [Anton](https://github.com/Jihadist) for the PR.
  * `.gitattributes` now handles line endings automatically for files detected
    as text. This simplifies WSL builds. Thanks to
    [Anatoly Shirokov](https://github.com/anatoly-spb) for the PR.
  * PostgreSQL libs selection is now possible in CMake if the platform has
    multiple versions installed, see
    @ref POSTGRES_LIBS "PostgreSQL versions" for more info.
  * Improved support for Arch Linux, many thanks to
    [Konstantin Goncharik](https://github.com/botanegg) for the PR.
* Multiple improvements for docs, including mockserver clarifications from
  [Victor Makarov](https://github.com/vitek).


### Beta (March 2023)

* Now logging of particular lines could be controlled by dynamic config. See
  @ref USERVER_LOG_DYNAMIC_DEBUG for more info.
* HTTP headers that contain the tracing context now could be customized,
  both for handlers and HttpClient, by feeding tracing::TracingManagerBase
  implementation into tracing::DefaultTracingManagerLocator (docs to come soon).
* User defined literals for different formats are now available at formats::literals
* Added crypto::CmsSigner and crypto::CmsVerifier as per RFC 5652.
* cache::LruMap and cache::LruSet now work with non-movable types.
* BSON<>JSON conversions now supported via
  json_value.ConvertTo<formats::bson::Value>() and
  bson_value.ConvertTo<formats::json::Value>().
* engine::CancellableSemaphore was implemented.
* Optimizations:
  * Mongo driver switched to a faster utils::statistics::Writer.
  * utils::Async functions now make 1 dynamic allocation less, thanks to
    [Ivan Trofimov](https://github.com/itrofimow) for the PR.
  * Getting the default logger now takes only a single atomic read. LOG_* 
    macro now do two RMW atomic operations less and do not use RCU, that could
    lead to a dynamic memory allocation in rare cases.
  * PostgreSQL driver now does much less atomic operations due to wider usage 
    of std::move on the internal std::shared_ptr.
  * Added storages::postgres::Transaction::ExecuteDecomposeBulk function for
    fast insertion of C++ array of structures as arrays of values.
  * Str[I]caseHash now uses a 5-10 times faster SipHash13
  * Redis driver now does an asynchronous DNS resolving, amount of heavy
    system calls dropped down noticeably.
* Build changes:
  * CMake option `USERVER_OPEN_SOURCE_BUILD` was removed as the build is always
    the same for in-house and public environments.
  * CMake option `USERVER_FEATURE_SPDLOG_TCP_SINK` was removed as now the
    implementation of the sink does not rely on spdlog implementation.
  * Configuration step was made much faster.
  * Makefile was simplified and only up-to-date targets were left.
  * Added a script to prepare docker build, see @ref scripts/docs/en/userver/docker.md for
    more info.
  * Scripts for generating CMakeLists were simplified and cleared from internal
    stuff.
  * Added missing dependencies to @ref scripts/docs/en/deps_ubuntu_.md20_04 and sorted all
    the dependencies, thanks to [Anatoly Shirokov](https://github.com/anatoly-spb)
    for the PR.
* Statistics and metrics now do additional lifetime checks in debug builds to
  catch improper usages.
* Push functions of concurrent::MpscQueue and other queues now have a
  `[[nodiscard]]` for compile time misuse detection.
* Significant improvement of Redis server-side errors diagnostic.
* Improved diagnostics for distributed locking.
* Fixed numerous typos in docs and samples.


### Beta (February 2023)

* tracing::Span now logs the location where it was constructed.
* Now the string<>enum utils::TrivialBiMap mappings could be used within
  storages::postgres::io::CppToUserPg. See @ref pg_enum for more info.
* components::Logging now provides options for configuring the task processors
  to do the asynchronous logging.
* Redis driver now supports the `geosearch` and `unlink` commands.
* DNS resolvers were switched to asynchronous mode by default.
* UDP chaos proxy was implemented, see
  @ref pytest_userver.chaos.UdpGate "chaos.UdpGate"
* C++ Standard now could be explicitly controlled via the CMake
  flag `CMAKE_CXX_STANDARD`.
* Optimizations:
  * Many metrics were moved to a faster utils::statistics::Writer.
  * gRPC now allows concurrent execution of 1 Read and 1 Write on the same
    Bidirectional stream.
* Python dependencies for build are now automatically installed, thanks to
  [Pavel Shuvalov](https://github.com/shuva10v) for the PR!
* Added a Conan option to disable LTO, thanks to
  [Oleg Burchakov](https://github.com/aarchetype) for the PR!
* Diagnostic messages and docs were improved, including:
  * In case of a typo in static config name the fixed name is now reported
  * `error-injection` static option for components::Postgres was documented
  * Fixed typos, thanks to [Ch0p1k3](https://github.com/Ch0p1k3) for the PR!


### Beta (January 2023)

* Unknown/mistyped values in static configs are now reported by default.
  @ref static-configs-validation "Static configs validation" now could use
  `minimum` and `maximum`.
* gRPC clients now have ReadAsync() functions, that return a future and
  allow to request multiple results from different RPCs at the same moment.
* ugrpc::server::Server now can return a vector of gRPC service names.
* To aid in asynchronous drivers development the engine::io::FdPoller is now
  a part of the public API.
* Added a blazing fast utils::TrivialBiMap.
* HTTP Streaming is now considered production ready, see
  @ref scripts/docs/en/userver/http_server.md for docs.
* Testsuite fixtures were improved:
  * Fixtures for detecting service readiness now work out of the box for
    services without server::handlers::Ping
  * Known databases do no not require manual dependency registration any more
  * Improved usage experience and testing of Metrics, see
    @ref TESTSUITE_METRICS_TESTING "Testsuite metrics testing".
  * Added fixtures to work with cache dumps.
  * @ref pytest_userver.plugins.config.service_secdist_path "service_secdist_path"
    and `--service-secdist` options simplify setup of the
    components::Secdist in tests.
  * Many fixtures were documented at
    @ref userver_testsuite_fixtures, pytest plugins were documented in
    tutorials and at @ref scripts/docs/en/userver/functional_testing.md.
* Optimizations:
  * Now the engine does less random number generator invocations for HTTP
    handling.
  * Logging of tracing::Span became x2 faster
  * IntrusiveMpscQueue is now used for engine internals, IO operations now
    schedule faster.
  * Writing HTTP headers became faster, thanks to
    [Ivan Trofimov](https://github.com/itrofimow) for the PR.
  * utils::TrivialBiMap is now used wherever it is possible.
* Metrics:
  * New interface for writing metrics utils::statistics::Writer is now
    considered stable.
  * More metrics were moved to a faster utils::statistics::Writer, including
    utils::statistics::MetricTag.
  * More metrics for Redis, including replica-syncing metrics.
  * More tests for metrics.
* FreeBSD build fixes.
* Multiple documentation and diagnostics improvements.
* Added `ToStringView(HttpMethod)` function, thanks to
  [Фёдор Барков](https://github.com/hitsedesen) for the PR.
* Added more engine::Yield tests, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the PR.


### Beta (December 2022)

* Added logs colorization to the testsuite plugins, thanks to
  [Victor Makarov](https://github.com/vitek) for the PR.
* Multiple big improvements in framework testing:
  * improved unit tests re-entrance
  * multiple new chaos and metrics tests
  * improved testsuite diagnostics
* Added a @ref scripts/docs/en/userver/tutorial/auth_postgres.md sample.
* Added an option `set_tracing_headers` to disable HTTP tracing headers, thanks
  to [Ivan Trofimov](https://github.com/itrofimow) for the PR.
* Fixed race in RabbitMQ sample, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the PR.
* Fixed PostgreSQL testing at GithubCI, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the PR.

### Beta (November 2022)

* engine::SingleWaitingTaskMutex was added, many thanks to the
  [Denis Korovyakovskiy](https://github.com/den818) for the PR!
* On-disk dumps were implemented for the LRU caches, many thanks to the
  [Roman Guryanov](https://github.com/YaImedgar) and
  [wyr241](https://github.com/wyr241) for the PR!
* Introduce metrics for ev-threads cpu load, thanks to the
  [Ivan Trofimov](https://github.com/itrofimow) for the PR.
* AMQP optimized, thanks to the [Ivan Trofimov](https://github.com/itrofimow)
  for the PR.
* Improved support for Conan packaging, many thanks to
  [Anton](https://github.com/Jihadist) for the PR.
* Fixed GCC build, thanks to the [Ivan Trofimov](https://github.com/itrofimow)
  for the PR.
* Fixed formats::yaml exception message typo, thanks to the
  [Denis Galeev](https://github.com/dengaleev) for the PR.
* Added gRPC testsuite mock sample at @ref scripts/docs/en/userver/tutorial/grpc_service.md.
* Added engine::TaskCancellationToken to simplify async drivers development.
* Simplified build steps, in particular:
  * Added `USERVER_PYTHON_PATH` to specify the path to the python3 binary for
    use in testsuite tests.
  * Conan patches were merged into CMake scripts.
  * Channelz support is now detected automatically.

### Beta (October 2022)

* Experimental support for Conan packaging, many thanks to
  [Anton](https://github.com/Jihadist) for the PR.
* Prometheus and Graphite metrics formats were added, see
  @ref scripts/docs/en/userver/service_monitor.md for details.
* Initial support for chaos testing was added, see
  @ref scripts/docs/en/userver/chaos_testing.md for more info.
* Generic Escape implementation for ranges was added to Clickhouse, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the PR.
* TLS/SSL support for Redis.
* Multiple optimizations from [Ivan Trofimov](https://github.com/itrofimow):
  * utils::datetime::WallCoarseClock and its usage in the framework core;
  * HTTP "Date" header caching;
  * Significant reduction of syscalls count during any recv operations;
  * Server::GetRequestsView not initialized if it is not required;
  * utils::encoding::ToHex became faster;
  * Marking response as ready became faster.
* Better diagnostics for CoroPool initialization failure, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the PR.
* New handler server::handlers::OnLogRotate.
* Multiple optimizations, including:
  * Faster async cancellations for PostgreSQL;
  * Avoid using dynamic_cast in multiple places;
  * Avoid calling `std::chrono::steady_clock::now()` in multiple places.
* gRPC mockserver support (see @ref scripts/docs/en/userver/tutorial/grpc_service.md).
* gRPC now provides an efficient API for async execution of requests without
  additional `utils::Async` invocations.
* Build fixes for older platforms, thanks to
  [Yuri Bogomolov](https://github.com/ybogo) for the PR.
* components::Logging now can output logs to UNIX sockets.
* Now the "help wanted" issues at github have additional tags "good first issue"
  and "big", to help you to choose between a good starting issue and a big
  feature. See @ref scripts/docs/en/userver/development/releases.md for more info.


### Beta (September 2022)

* [Ivan Trofimov](https://github.com/itrofimow) implemented the RabbitMQ driver.
* Added navigation to the next and previous page in docs, thanks to multiple
  feature requests in [Telegram support chat](https://t.me/userver_ru).
* Improved Task::Detach docs and added recommendation to use
  concurrent::BackgroundTaskStorage instead, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the bugreport.
* Added `start-` targets for the samples, to simplify experimenting with them.
* Docs now support dark theme out of the box.
* Fixed CMake issue with `-DUSERVER_FEATURE_TESTSUITE=0`, thanks to
  [Георгий Попов](https://github.com/JorgenPo) for the bugreport.
* Fixed build on Arch Linux, thanks to [Mikhail K.](https://github.com/posidoni)
  for the bugreport.
* Fixed building in virtual environment on Windows, thanks to
  [sabudilovskiy](https://github.com/sabudilovskiy) for the bug report.
* Fixed building with `-std=gnu++20`, thanks to
  [Георгий Попов](https://github.com/JorgenPo) for the PR. 
* Improved package version detection in CMake via `pkg-config`.
* Added a `USERVER_FEATURE_UTEST` flag for disabling utest and ubench target
  builds, thanks to [Anton](https://github.com/Jihadist) for the PR.
* Simplified gRPC component registration and usage.
* Added an ability to turn on gRPCs ChannelZ.
* Added evalsha/script load commands for Redis driver.


### Beta (August 2022)

* Added server::handlers::HttpHandlerStatic handler to serve static pages.
* Added navigation to previous and next page in docs.
* Optimized internals:
  * WaitListLight now never calls std::this_thread::yield().
  * More lightweight queues are now used in HTTP server.
  * Smaller critical section in TaskContext::Sleep(), improved performance for
  many the synchronization primitives.
  * std::unique_ptr now holds the payload in engine::Task, rather than
  std::shared_ptr. Thanks to the [Stas Zvyagin](https://github.com/szvyagin-gj)
  for the idea and draft PR.
  * Simplified and optimized FdControl, resulting in less CPU and memory usage
  for sockets, pipes and TLS.
* Fixed typos in tests, thanks to [Георгий Попов](https://github.com/JorgenPo)
  for the PR.
* Removed suspicious operator, thanks to the
  [PatriotRossii](https://github.com/PatriotRossii) for the bugreport.
* Fixed CentOS 7.8 builds, many thanks to [jinguoli](https://github.com/jinguoli)
  for the bugreport and fix ideas.
* Fixed Gentoo builds, many thanks to [SanyaNya](https://github.com/SanyaNya)
  for the PR.
* Fixed default DB values in
  [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf),
  many thanks to [skene2321](https://github.com/skene2321) for the bugreport.
* Added engine::io::WritableBase and engine::io::RwBase, thanks to
  [Stas Zvyagin](https://github.com/szvyagin-gj) for the idea.
* Added components::TcpAcceptorBase with new tutorials
  @ref scripts/docs/en/userver/tutorial/tcp_service.md and @ref scripts/docs/en/userver/tutorial/tcp_full.md,
  thanks to [Stas Zvyagin](https://github.com/szvyagin-gj) for the idea and
  usage samples at https://github.com/szvyagin-gj/unetwork.
* Fixed comparison operator for UserScope, thanks to
  [PatriotRossii](https://github.com/PatriotRossii) for the PR.
* Add CryptoPP version download during CryptoPP installation, thanks to
  [Konstantin](https://github.com/Nybik) for the PR.
* Added more documentation on Non FIFO queues, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the report.
* Added missing std::atomic into TaskProcessor, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the report.
* Fixed Boost version detection on MacOs, thanks to
  [Konstantin](https://github.com/Nybik) for the PR.
* Added Fedora 36 support, thanks to
  [Benjamin Conlan](https://github.com/bjconlan) for the PR.
* Improved statistics for gRPC.
* Vector versions of engine::io::Socket::SendAll were added and used to
  optimize CPU and memory consumption during HTTP response sends.


### Pre announce (May-Jul 2022)

* Fixed engine::io::TlsWrapper retries,
  thanks to [Ivan Trofimov](https://github.com/itrofimow) for the report.
* Fixed missing `const` in utils::DaemonMain function,
  thanks to [Denis Chernikov](https://github.com/deiuch) for the PR.
* Cmake function `userver_testsuite_add` now can pass arguments to `virtualenv`,
  thanks to [Дмитрий Изволов](https://github.com/izvolov) for the PR.
* Improved hello_service run instruction,
  thanks to [Svirex](https://github.com/Svirex) for the PR.
* Better Python3 detection, thanks to
  [Дмитрий Изволов](https://github.com/izvolov) for the PR.
* Task processors now have an `os-scheduling` static config option and
  @md_en_userver_task_processors_guide "a usage guide".
* Added a [pg_service_template](https://github.com/userver-framework/pg_service_template)
  service template that uses userver the userver framework with PostgreSQL
* In template services, it is now possible to deploy the environment and run
  the service in one command:  `make service-start-debug` or
  `make service-start-release`.
* Added userver::os_signals::Component, which is used for handling OS
  signals.
* You can now allow skipping the component in the static config file by
  specializing the components::kConfigFileMode,
  see @ref scripts/docs/en/userver/component_system.md "the documentation".
* The PostgreSQL driver now requires explicit serialization methods when
  working with enum.
* Optimized CPU consumption for handlers that do not log requests or responses.
* utils::Async() now can be invoked from
  non-coroutine thread (in that case do not forget to use
  engine::Task::BlockingWait() to wait for it). tracing::Span construction
  became faster.
  Thanks to [Ivan Trofimov](https://github.com/itrofimow) for the report.
* Improved MacOS support, thanks to
  [Evgeny Medvedev](https://github.com/kargatpwnz).
* Docker support: [base image for development](https://github.com/userver-framework/docker-userver-build-base/pkgs/container/docker-userver-build-base),
  docker-compose.yaml for the userver with build and test targets.
  See @ref scripts/docs/en/userver/tutorial/build.md
* Docs improved: removed internal links; added
  @ref scripts/docs/en/userver/framework_comparison.md,
  @ref scripts/docs/en/userver/supported_platforms.md,
  @ref scripts/docs/en/userver/security_changelog.md,
  @ref scripts/docs/en/userver/profile_context_switches.md,
  @ref scripts/docs/en/userver/driver_guide.md,
  @md_en_userver_task_processors_guide,
  @ref scripts/docs/en/userver/os_signals.md and @ref scripts/docs/en/userver/roadmap_and_changelog.md.
* AArch64 build supported. Tests pass successfully
* HTTP headers hashing not vulnerable to HashDOS any more, thanks to Ivan
  Trofimov for the report.
* engine::WaitAny now can wait for engine::Future, including futures that are
  signaled by engine::Promise from non-coroutine environment. 
* Optimized the PostgreSql driver, thanks to Dmitry Sokolov for the idea.
* Arch Linux is now properly supported, thanks to
  [Denis Sheremet](https://github.com/lesf0) and
  [Konstantin Goncharik](https://github.com/botanegg).
* Published a service to manage
  [dynamic configs](https://github.com/userver-framework/uservice-dynconf) of
  other userver-based services.
* Now it is possible to enable logging a particular LOG_XXX by its source
  location, see server::handlers::DynamicDebugLog for more details.
* Added a wrapper class utils::NotNull and aliases utils::UniqueRef and
  utils::SharedRef.
* LTSV-format is now available for logs via components::Logging static option


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/development/releases.md | @ref scripts/docs/en/userver/faq.md ⇨
@htmlonly </div> @endhtmlonly

