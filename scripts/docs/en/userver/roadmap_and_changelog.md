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

* 👨‍💻 gRPC simplification and functionality improvement.
* gRPC: @ref scripts/docs/en/userver/deadline_propagation.md "Deadline propagation" support for absolute
  deadline metadata (`x-request-deadline`) in addition to duration deadlines (`grpc-timeout`); see @ref
  scripts/docs/en/userver/grpc/grpc.md.
* Keep improving the @ref scripts/docs/en/userver/chaotic.md


## Changelog


### Release v3.0

Big new features since the v2.0:

* @ref scripts/docs/en/userver/chaotic.md "chaotic" - codegen parsers and serializers by JSON schema
* @ref @ref scripts/docs/en/userver/libraries/easy.md
* Improved Kafka driver is now widely used and has @ref QUALITY_TIERS "Platinum Tier".
* @ref opentelemetry "OpenTelemetry Protocol (OTLP) support".
* Logging format customization, including JSON logging.
* Numerous build improvements, including MacOS, Gentoo, Arch, Alpine. Improved Conan support.
* @ref scripts/docs/en/userver/sqlite/sqlite_driver.md
* Web interface for the [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf)
* HTTP 2.0 basic server support
* Dropped C++17 support.
* Improved documentation, added more samples and descriptions.
* Numerous optimizations.

Binary Ubuntu 24.04 and Ubuntu 22.04 amd64 packages could be found at
[userver Releases](https://github.com/userver-framework/userver/releases).

Detailed diff against v2.16 could be found below.

### v3.0-rc

* **Support of C++17 is removed**.
* Updated moodycamel to 1.0.5. Many thanks to [Vasily Sviridov](https://github.com/vasily-sviridov) for the PRs.
* Supported @ref ydb::TableClient::RetryTx handle for execution lambda in a transaction with automatic retries.
* @ref scripts/docs/en/userver/libraries/multi_index_lru.md is now optimized and ready to use. Many thanks to
  [hzhdlrp](https://github.com/hzhdlrp) for the contributions!
* Added heartbeat and message headers support for RabbitMQ. Many thanks to [Savochkin Denis](https://github.com/sav-da).
* Added URL encoding for S3 object keys. Many thanks to [Tnirpps](https://github.com/Tnirpps) for the PR!
* Fixed `-Wc99-extensions` warning in PostgreSQL driver. Many thanks to
  [Dmitry Kopturov](https://github.com/IsThisLoss).
* Added the @ref kafka::Producer::Send() method for synchronous batch message delivery to a specified Kafka topic.
  Implementation spawns only one coroutine for the entire message batch increasing the efficiency of the driver.
  Driver now attempts to handle `librdkafka` local queue overflow errors within the configured `delivery.timeout.ms`
  limit. Many thanks to [Dmitry Isaikin](https://github.com/disaykin) for the PR!
* @ref utils::statistics::MonotonicByLabelStorage used for metrics in HTTP client, leading to an order of magnitude
  faster insertion of new metrics.
* Added `[[lifetimebound]]` annotations. More lifetime errors in user code is now detected at compile time.
* Modernized the code, replacing multiple of `std::enable_if` with C++20 concepts. Some of the very rarely used
  template variables were replaced with concepts without the `k` prefix. User code usually not affected, but here's a
  migration hint for the unfortunate ones:
  ```
  find . -type f -name '*pp' | xargs sed -i \
      -e 's/kIsMappedToArray/IsMappedToArray/g' \
      -e 's/kIsMappedToUserType/IsMappedToUserType/g' \
      -e 's/io::traits::kHasParser/io::traits::HasParser/g' \
    	-e 's/io::traits::HasFormatter/io::traits::HasFormatter/g' \
      -e 's/kCanReserve/CanReserve/g' \
      -e 's/kCanResize/CanResize/g' \
      -e 's/kCanClear/CanClear/g' \
      -e 's/traits::kIsFixedSizeContainer/meta::kIsFixedSizeContainer/g' \
  ```
* HTTP: @ref scripts/docs/en/userver/deadline_propagation.md "Deadline propagation" support for an absolute
  deadline timepoint via the `X-Request-Deadline` header in addition to `X-YaTaxi-Client-TimeoutMs`.


### Release v2.16

* **Support of C++17 is deprecated and will be removed in one on the next releases**. C++20 is the default for
  userver for quite some time. Use C++20 or even a more modern C++.
* New readiness notification flow for tasks, futures, events and other awaitable objects. It provides a lightweight and
  efficient framework for more efficient implementation of waiting objects/methods like @ref engine::WaitAny,
  @ref engine::io::FdPoller, @ref engine::TaskBase::BlockingWait .
* Ydb: @ref ydb::Transaction and `ydb::TableClient::ExecuteDataQuery` switched from Table Client to Query Client API.
* Multicast support for sockets. See @ref engine::io::IpMreq .
  Many thanks to [Dmitry Borchuk](https://github.com/dmitryborchuk) for the contribution!
* SAX parsing in Chaotic codegen. See @ref scripts/docs/en/userver/chaotic.md .
* HttpClient: New Websocket client. See @ref scripts/docs/en/userver/tutorial/websocket_client.md .
* gRPC: Retry limiter. See @ref ugrpc::client::RetryLimiter .

* Separate log for components loading that simplifies components loading issues debugging.
  See `components_manager.boot-log-path` in @ref components::ManagerControllerComponent .
* Redis: Valkey and Redis dynamic client component. See @ref components::DynamicRedis .
* @ref components::StatisticsStorage allows filtering of metrics by @ref utils::statistics::MetricTag .
* Automatic conversions between structs and JSON in `easy` library. See @ref scripts/docs/en/userver/libraries/easy.md .
* Resource scopes separated from the component system. See @ref utils::ResourceScopeStorage .
* New @ref multi_index_lru::ExpirableContainer container. Many thanks to [hzhdlrp](https://github.com/hzhdlrp)
  for the contribution!
* Kafka: Added `group_instance_id` option to @ref kafka::ConsumerComponent .
* gRPC: Perform retry on the next channel.
* Ydb: New transaction modes support. See `ydb::TransactionMode`.
* Ydb: New AsContainer() method in cursor. See @ref ydb::Cursor::AsContainer .
* Redis: New @ref storages::redis::Client::EvalReadOnly and @ref storages::redis::Client::EvalShaReadOnly functions.
* Per-handler metrics in HTTP server. See `SetHandlerMetricsShard` in @ref server::request::RequestContext .
* gRPC: Extended gRPC client completion statuses. See @ref ugrpc::client::SpecialCaseCompletionType .
* `operator==` for proto structs.
* Userver can listen on a socket passed via `execve`. See `listen-socket-fd` parameter description in
  @ref components::Server .
* New `contains_no_update` method in @ref multi_index_lru::Container and benchmarks cleanup.
  Many thanks to [hzhdlrp](https://github.com/hzhdlrp) for the contribution!
* New @ref net::blocking::ConnectTcpByName method to connect to a remote endpoint via a host name and a port.
* Converters between proto structs and binary data. See @ref userver/proto-structs/convert.hpp .
* Converters between proto structs and JSON. See @ref userver/proto-structs/json.hpp .
* Converters between proto structs enums and strings.
* @ref server::websocket::WebSocketConnection support for waiting primitives (like @ref engine::MakeWaitAny ).
* Added @ref engine::io::Sockaddr::MakeIPSocketAddress, @ref engine::io::Sockaddr::MakeInaddrAny and
  @ref engine::io::Sockaddr::MakeIPv4InaddrAny functions.
  Many thanks to [Dmitry Borchuk](https://github.com/dmitryborchuk) for the contribution!

* Optimizations and fixes
  * New @ref engine::WaitAnyContext (@ref engine::MakeWaitAny ) and @ref engine::WaitAllChecked waiting primitives
    optimized for usage in loops.
  * Kafka: Reuse consumer after a user exception.
  * Redis: Optimized statistics storage - removed most of per-shard and per-instance statistics that are rarely used
    to save memory, CPU and network bandwidth.
  * Prevent an unnecessary copy inside the `ServerImpl::WriteTotalHandlerStatistics` function.
    Many thanks to [gabrielyanabraham](https://github.com/gabrielyanabraham) for the PR!
  * Optimized @ref engine::TaskBase::BlockingWait .
  * Kafka: Deprecated @ref engine::WaitAny replaced with @ref engine::MakeWaitAny .
    Many thanks to [Dmitry Isaikin](https://github.com/disaykin) for the PR!
  * Fixed @ref yaml_config::YamlConfig mode when accessing an array by index.
    Many thanks to [Fedor Shatokhin](https://github.com/FedShat) for the contribution!
  * Fixed compilation error with modern `libfmt` versions. Many thanks to
    [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Fixed -flto=auto support for GCC. Many thanks to [Konstantin Goncharik](https://github.com/botanegg)
    for the PR!
  * Fixed broken `journal_mode` and `synchronous` options definitions in @ref components::SQLite .
    Many thanks to [Alexey Mednyy](https://github.com/swex) for the PR!
  * Fixed a compile warning in @ref utils::PeriodicTask .
    Many thanks to [Evgeny Proydakov](https://github.com/proydakov) for the PR!
  * HttpClient: Fixed socket type equality check. Many thanks to [Konstantin Goncharik](https://github.com/botanegg)
    for the PR!
  * Ydb: Fixed compilation error with modern `libfmt` versions.
    Many thanks to [Vasily Sviridov](https://github.com/vasily-sviridov) for the PR!

* Build and testing
  * Updated Google benchmark to 1.9.5. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Userver dev container. See @ref scripts/docs/en/userver/build/userver.md .
  * New `component_manager.coro_pool.unoptimized_stack_size_multiplier` option to automatically increase stack size
    in unoptimized builds. See @ref components::ManagerControllerComponent .
  * gRPC: @ref pytest_userver.grpc._client.PreCallClientInterceptor accessible from testsuite.
  * New @ref utils::StartPeriodicTask method that allows deterministic testing of periodic tasks in testsuite.
  * Upgraded CPM.cmake to 0.42.1. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!

* Documentation
  * Better documentation and benchmark for LRU caches.
  * Markdown documentation for cmake files.
  * Postgres: Updated connections limit documentation. See @ref scripts/docs/en/userver/pg/connlimit_mode_auto.md .
  * Extra samples for metrics unit testing.
  * Updated @ref engine::SingleUseEvent and @ref engine::SingleConsumerEvent documentation.
  * gRPC: @ref ugrpc::RichStatus documentation. See @ref scripts/docs/en/userver/grpc/rich_status.md .
  * Added missing include to @ref README.md. Many thanks to [Aleksey Belov](https://github.com/belov-aleksey) for the PR!
  * Updated Gentoo build docs. Many thanks to [halfdarkangel](https://github.com/halfdarkangel) for the contribution!
  * Detailed description of Congestion Control logic. See @ref scripts/docs/en/userver/congestion_control.md .
  * Kafka: Added `pool_timeout` to kafka-consumer config in @ref samples/kafka_service/static_config.yaml .
    Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!


### Release v2.15

* **Support of C++17 is deprecated and will be removed in one on the next releases**. C++20 is the default for
  userver for quite some time. Use C++20 or even a more modern C++.
* Deadlock detector is now enabled by default in testsuite runs. The behavior can be changed by overriding the
  @ref pytest_userver.plugins.config.userver_deadlock_detector_mode() fixture.
* Redis driver internals were rewritten to remove duplicate code in `Sentinel`/`ClusterImpl`, simplify inheritance and
  reduce binary size.
* New `pull-pin-task-queue` experimental scheduler where each task gets pinned to a thread-specific
  queue and is executed only in that thread. See static config option 'task-processor-queue' of the
  @ref components::ManagerControllerComponent for more info.
* Drastically reduced memory usage by @ref components::Redis statistics/metrics in case of network topology changes or
  multiple Valkey/Redis nodes going down.
* Stacktrace capturing via `boost::stacktrace` / `std::stacktrace` in Boost.Context became more than x50 faster on
  LLVM version of `libunwind`. Other unwinding libraries could have also gained profit.
* gRPC now clamps status codes outside the valid `[0, 16]` range to `UNKNOWN`.
* More docs for the @ref USERVER_LOG_DYNAMIC_DEBUG, logging macros and @ref engine::SharedMutex.
* Documented the metrics testing and creation. See @ref scripts/docs/en/userver/metrics.md
* Added `use_secure_connection` static config option for @ref storages::mysql::Component.
  Many thanks to [Yury Bogomolov](https://github.com/ybogo) for the PR.
* Workaround `+=` use for `std::atomic<double>` for llvm-17/libc++. Many thanks to
  [Alexander Chernov](https://github.com/blackav) for the PR.
* Added missing `<ctime>` include. Many thanks to [Alexander Chernov](https://github.com/blackav) for the PR.
* Added missing `<iterator>` include. Many thanks to [Alexander Chernov](https://github.com/blackav) for the PR.
* Added missing `<fmt/ranges.h>` include. Many thanks to [Taras Litvinenko](https://github.com/xensmo) for the PR.
* Fixed flapping YDB tests. Many thanks to [Bulat Gayazov](https://github.com/Gazizonoki) for the PR!


### Release v2.14

* Added a @ref ugrpc::RichStatus builder for creating rich gRPC error statuses with
  structured details following Google's error model
* Added a @ref utils::statistics::RegisterWriterScope for safe and easy registration of statistics.
* No more need to call `CacheUpdateTrait::StartPeriodicUpdates()` from your caching components.
* Implemented a @ref scripts/docs/en/userver/libraries/multi_index_lru.md, thanks to
  [hzhdlrp](https://github.com/hzhdlrp).
* Statement name is now logged when using @ref storages::postgres::Portal.
* Monitor port is now opened before all the components and handlers are started, leading to more control and
  information on service start.
* @ref storages::postgres::DistLockComponentBase now can be configured at runtime via @ref POSTGRES_DISTLOCK_SETTINGS.
* Added @ref utils::TaskBuilder
* ‎@ref scripts/docs/en/userver/libraries/s3api.md now has @ref QUALITY_TIERS "Platinum Tier".
* Kafka is now more accurate in computing message time spent in @ref kafka::ConsumerComponent queue.
* @ref utils::statistics::RecentPeriod now does not lose precision under heavy contention.
* Added functionality to @ref scripts/docs/en/userver/dump_coroutines.md
* Added DeadlockDetector that can be enabled via `coro_pool.deadlock_detector` option
  in @ref components::ManagerControllerComponent.
* Added @ref storages::mongo::Transaction
* Added @ref server::middlewares::Cors
* Added @ref utils::FromStringNoThrow
* Fixed unnecessary lock in TaskContext destructor when there are no plugins. Many thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the PR
* @ref formats::yaml::Value now checks for key uniqueness in mappings.
* @ref scripts/docs/en/userver/gdb_debugging.md "Stack monitor usage" is now automatically disabled under GDB.
* Disabled phdr cache if ASAN is enabled. Many thanks to
  [Yury Bogomolov](https://github.com/ybogo) for the PR
* Allowed using mapped enums in @ref storages::postgres::ParameterStore. Many thanks to Aleksey Popov for the patch and
  help!
* Fixed error in [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf) schema. Many thanks to
  [Repin Daniil](https://github.com/Repin-Daniil) for the PR!
* @ref components::ComponentContext::Scopes() now can be used to register some resource that will be
  called after the component is successfully created (including all
  class descendants) and destroyed right before calling the destructor of the most derived component. This is a low
  level feature, more high level functions for caches, distlocks and subscriptions will appear soon.

* Build
  * The oldest supported Python is now Python 3.10.
  * Build fixes for PostgreSQL >= 17 on Ubuntu. Many thanks to [Pavel Sidorovich](https://github.com/RayeS2070)
    for the initial PR!
  * Chaotic golden tests now use clang-format on both sides of the diff to avoid failures dues to clang-format version
    changes. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * The correct stable version of devcontainer is now presumed by `userver-create-service` script, resulting in stable
    work of the service on older containers.
  * Added `rdkafka_mock.h` header to workaround uncommon header location in some of the build environments.
    Many thanks to [Vitalii](https://github.com/beryll1um) for the PR!
  * Removed USERVER_HTTP_PROXY dynamic config
  * Multiple updates to CI packet versions, build processes and docker container creation.

* Documentation and diagnostics
  * Tables with static configuration options are now build from component schemas. As a result the options description
    became more accurate.
  * Added @ref pytest_userver.plugins.service.service_start_timeout "service_start_timeout" fixture into testsuite.
    Many thanks to [DmitriyH](https://github.com/DmitriyH) for the PR!
  * Added multipart methods support for testsuite @ref pytest_userver.plugins.s3api "s3api plugin".
  * @ref utils::FromString() now reports if input sequence of chars to convert contains '\0' character.
  * Improved diagnostics for PostgreSQL composite types related errors, including nested composite types.
  * Fixed typo in third party. Many thanks to [shuric80](https://github.com/shuric80) for the PR!
  * Fixed typo in Docker badge link for upastebin. Many thanks to [Гуща Иван](https://github.com/Ivan160927777)
    for the PR!
  * A lot of updates for different sections of documentation, including @ref utils::FixedArray,
    @ref scripts/docs/en/userver/deadline_propagation.md, @ref clients::http::Request,
    @ref scripts/docs/en/userver/gdb_debugging.md, @ref scripts/docs/en/userver/long_transactions.md,
    @ref scripts/docs/en/userver/mongodb.md, @ref scripts/docs/en/userver/congestion_control.md


Plugins are renamed to middlewares and @ref components::HttpClient was split into @ref components::HttpClient
  and @ref components::HttpClientCore.

#### Migration guide from v2.13 to v2.14

1. Instead of `.Append<components::HttpClient>()` use `.AppendComponentList(clients::http::ComponentList())` in
   your component list (see @ref clients::http::ComponentList "docs").
2. For classes inherited from @ref tracing::TracingManagerBase :
   1. Change @ref clients::http::PluginRequest in @ref tracing::TracingManagerBase::FillRequestWithTracingContext()
      parameters to @ref clients::http::MiddlewareRequest
3. For classes inherited from @ref clients::http::Plugin :
   1. Replace `#include <userver/clients/http/plugin.hpp>` with `#include <userver/clients/http/middlewares/base.hpp>`
   2. Change base class to @ref clients::http::MiddlewareBase
   3. Change clients::http::PluginRequest in methods parameters to @ref clients::http::MiddlewareRequest
   4. Stop passing middleware name to  base class constructor
   5. Empty methods implementations (or `return true;` for @ref clients::http::MiddlewareBase::HookOnRetry()) may be omitted

   For example:

   ```
   # diff
   -#include <userver/clients/http/plugin.hpp>
   +#include <userver/clients/http/middlewares/base.hpp>
   ...

   -class MyPlugin : public clients::http::Plugin {
   +class MyMiddleware : public clients::http::MiddlewareBase {
   public:
   ...
   -    MyPlugin() : clients::http::Plugin("my-plugin") {}
   +    MyMiddleware() : clients::http::MiddlewareBase() {}

   -    void HookPerformRequest(clients::http::PluginRequest& request) override { do_something(request); }
   +    void HookPerformRequest(clients::http::MiddlewareRequest& request) override { do_something(request); };

   -    void HookCreateSpan(clients::http::PluginRequest&, tracing::Span&) override {}
   -    void HookOnCompleted(clients::http::PluginRequest&, clients::http::Response&) override {}
   -    void HookOnError(clients::http::PluginRequest&, std::error_code) override {}
   -    bool HookOnRetry(clients::http::PluginRequest&) override { return true; }
   ...
   };
   ```
4. For plugin components inherited from @ref clients::http::plugin::ComponentBase :
   1. Replace `#include <userver/clients/http/plugin_component.hpp>` with `#include <userver/clients/http/middlewares/component.hpp>`
   2. Change base class to @ref clients::http::middlewares::ComponentBase
   3. Component name is not required to have prefix `http-client-plugin-` anymore, `http-client-` prefix is suggested instead
   4. Rename `GetPlugin` method to `GetMiddleware`, change its return type to `clients::http::MiddlewareBase`
   5. Use @ref dynamic\_config::Source::UpdateAndListen() to subscribe on dynamic config updates
   6. Pass middleware index to base class constructor, instead of setting it in config file

   For example:

   ```
   # diff
   -#include <userver/clients/http/plugin_component.hpp>
   +#include <userver/clients/http/middlewares/component.hpp>
   ...

   -class SomeComponentName : public clients::http::plugin::ComponentBase {
   +class SomeComponentName : public clients::http::middlewares::ComponentBase {
   public:
   -    static constexpr std::string_view kName = "http-client-plugin-my-plugin-name";
   +    static constexpr std::string_view kName = "http-client-my-middleware-name";

   -    SomeComponentName(const components::ComponentConfig& config, const components::ComponentContext& context)
   -        : ComponentBase(config, context),
   -          plugin_(std::make_unique<Plugin>()),
   +    SomeComponentName(const components::ComponentConfig& config, const components::ComponentContext& context)
   +        : ComponentBase(config, context, clients::http::middlewares::MiddlewareIndex{1234}),
   +          middleware_(std::make_unique<Middleware>()),
   {
       auto& config_component = context.FindComponent<components::DynamicConfig>();
       subscriber_scope_ =
   -        components::DynamicConfig::NoblockSubscriber{config_component}
   -            .GetEventSource()
   -            .AddListener(this, kName, &Component::OnConfigUpdate);
   +        config_component
   +            .GetSource()
   +            .UpdateAndListen(this, kName, &Component::OnConfigUpdate);
   }
   ...
   -    clients::http::Plugin& GetPlugin() override { return *plugin_; }
   +    clients::http::MiddlewareBase& GetMiddleware() override { return *middleware_; }

   private:
   -    std::unique_ptr<clients::http::Plugin> plugin_;
   +    std::unique_ptr<clients::http::MiddlewareBase> middleware_;
   };
   ```
5. Rename `http-client:` to `http-client-core:` in static config.
6. If you are using or customized plugins/middlewares:
   1. In every @ref components::HttpClient config, rename `plugins` to `middlewares`
   2. Update middleware component names to current `Component::kName`
   3. Use full middleware components names as keys and `{enabled: true}` as values in `middlewares` for every @ref components::HttpClient config
   4. Move middlewares that you want to use for all @ref components::HttpClient instances from `http-client.middlewares` to `http-client-middleware-pipeline.middlewares`
   5. Set `dynamic-config-http-client.middlewares.<middleware name>.enabled` to `false` for all middlewares that are subscribed to dynamic configs updates

   For example:

   ```
   # diff
   components_manager:
       components:
   -        http-client-plugin-common-plugin-name:
   +        http-client-common-middleware-name:
               ...
   -        http-client-plugin-plugin-with-dynamic-config:
   +        http-client-middleware-with-dynamic-config:
               ...

   -        http-client:
   +        http-client-core:
                fs-task-processor: fs-task-processor
   -            plugins:
   -                common-plugin-name: 1
   -                with-dynamic-config: 2
   +        http-client-middleware-pipeline:
   +            middlewares:
   +                http-client-common-middleware-name:
   +                    enabled: true
   +                http-client-middleware-with-dynamic-config:
   +                    enabled: true
   +        dynamic-config-http-client:
   +            middlewares:
   +                http-client-middleware-with-dynamic-config:
   +                    enabled: false
   ```
7. Remove calls to `CacheUpdateTrait::StopPeriodicUpdates()`. Remove calls to `CacheUpdateTrait::StartPeriodicUpdates()`
   or replace them with @ref CacheUpdateTrait::EarlyStartPeriodicUpdates() if early update is required.


### Release v2.13

* Recursive subscriptions on dynamic configs now work without deadlocks.
* Added virtual method @ref server::handlers::HttpHandlerBase::GetUrlForLogging() for custom URL logging.
* Support parsing of HTTP keys without `=`, i.e. `/endpoint?key1&key2`. Many thanks to
  [Norlin](https://github.com/mistergad) for the PR!
* Dots in tag keys of logs and spans are now not changed to underscores. HTTP client/server spans are now written
  according to OTel conventions.
* Added support for ReplyTo, CorrelationId, Expiration fields. Many thanks to
  [Alexander Aparin](https://github.com/alex-aparin) for the PR!
* @ref s3api::Client now supports multipart upload.
* Redis now understands the `GEOPOS` and `EXPIRE` commands via @ref storages::redis::Client member function `Geopos()`
  and `Expire()`.
* Thread Sanitizer suppression file `cmake/tsan.suppressions.txt` is now installed and used in internal CI TSan tests
  for universal, core and gRPC.
* @ref logging::LogExtra::Value now supports `bool` values. Many thanks to
  [Maxim Drugov](https://github.com/crystalixxx) for the PR!
* YDB `topic_client` is now exposed in testsuite's @ref pytest_userver.plugins.ydb.client.YdbClient "YdbClient".
* Removed `MONGO_DEADLINE_PROPAGATION_ENABLED_V2` dynamic config. Many thanks to [Spar](https://github.com/GitSparTV)
  for vibe-coding the PR at Zero Cost Conf!
* @ref yaml_config::YamlConfig now treats `#env` values as string.

* Optimizations
  * TSKV based formatters are now constructed 3 times faster leading to about 1% overall improvement for CPU
    consumption of some services.
  * Trace data extraction and processing is now 10 times faster.
  * Optimized @ref logging::LogExtra efficiency with `std::initializer_list` to avoid memory allocations.
  * @ref fs::blocking::CFile now does not lock internal mutex when works with files. Up to 0.2% improvement in logging
    and ~40% improvement for dumping caches of integers.
  * Reuse `SSL_CTX` for TLS/SSL connections. As a result memory consumption drastically reduced for TLS/SSL server with.
    Many thanks to [DenisRazinkin](https://github.com/DenisRazinkin) for the awesome PR!

* Build
  * Fix compiling with `fmt` >= 12. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Improved compatibility of response body for aws sdk clients. Many thanks to
    [Alexander Aparin](https://github.com/alex-aparin) for the PR!
  * Support libpq patching with pq >= 18. MAny thanks to [Vladislav Nepogodin](https://github.com/vnepogodin) for
    the PR!
  * `api-common-protos` are now used from the main branch of the upstream, rather than from `1.50` version.
    Many thanks to [Alexey Medvedev](https://github.com/Reavolt) for the PR!
  * Support for compiler GCC-9 and below was dropped.
  * Fix issue with pip call ignored args specified in USERVER_PIP_OPTIONS. Many thanks to
    [Komar Dmitry](https://github.com/KomarDL) for the PR!
  * Fix building with CPM downloaded abseil-cpp. Many thanks to [Konstantin Goncharik](https://github.com/botanegg)
    for the PR!
  * Added support for Protobuf v6 (30.x). Many thanks to
    [Vasily Sviridov](https://github.com/vasily-sviridov) for the PR!
  * Third-party date/date.h was updated. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Fixed missing namespace for @ref scripts/docs/en/userver/codegen_overview.md "userver_embed_file".
    Many thanks to [Sergei Fedorov](https://github.com/zmij) for the PR!
  * Packaged installed via CPM are now printed at the end of configure stage.
  * Boost, c-ares, libev, libnghttp2, yaml-cpp, openssl now can be downloaded via CPM.
  * Added more tests for gRPC in subdirectories. Many thanks to [Aleksey Ignatiev](https://github.com/ae-ignatiev)
    for providing test samples.
  * Added CI runs on Arch. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Added CI runs on Debian.

* Documentation and diagnostics
  * Use SFINAE for @ref utils::MakeSharedRef() and @ref utils::MakeUniqueRef() to simplify diagnostics. Many thanks
    to [Pantus Raman](https://github.com/NiskashY) for the PR!
  * Improved deadlock detection for @ref engine::Mutex.
  * Set span level=debug for annoying ping queries. Many thanks to [j4niwzis](https://github.com/j4niwzis) for the PR!
  * Clarified ownership of buckets in @ref utils::statistics::Histogram.
  * Added mirrors to the videos at @ref scripts/docs/en/userver/publications.md.
  * Redis now provides much more information in logs.
  * Added troubleshooting info on `SIGUSR1` to @ref scripts/docs/en/userver/faq.md .


### Release v2.12

* Added infrastructure to detect IO-bound operations in transactions. See `enable_trx_tracker` static config option into
  @ref components::ManagerControllerComponent and @ref userver/utils/trx_tracker.hpp. The detection is turned on by
  default and is used in HTTP clients and gRPC for PostgreSQL transactions.
* Added support for separate logs and traces endpoints into OpenTelemetry. See `logs-endpoint` and `tracing-endpoint`
  static config options of the @ref otlp::LoggerComponent.
* Implemented non blocking receive for engine::io::TlsWrapper. Many thanks to
  [Denis Razinkin](https://github.com/DenisRazinkin) for the PR!
* Added @ref server::websocket::WebSocketConnection now has `SendPing()` and `NotAnsweredSequentialPingsCount()`
  functions for initiating pings on server side.
* Created [userver mirror repo at SourceCraft](https://sourcecraft.dev/userver/repos).

* Kafka now supports SSL authentication. Many thanks to [Timur Yalymov](https://github.com/tyalymov) for the PR!
* Fixed UB in WebSockets. Many thanks to [Denis Razinkin](https://github.com/DenisRazinkin) for the PR!
* Chaotic now handles `x-usrv-cpp-container: std::unordered_set` with `format: uuid`;
  `x-usrv-cpp-type: userver::utils::StrongTypedef<my::CustomString, std::string>`; `std::intXX_t`
* Crypto part of the `userver::universal` now supports work with multiple `std::string_views`. Thanks to
  [Dmitry Sorokin](https://github.com/sorydima) for vibe-coding the initial version of PR.
* Google Benchmark ASLR-disabling feature enabled if supported. Many thanks to
  [Konstantin Goncharik](https://github.com/botanegg) for the PR!
* Simplified third-party types usage in chaotic. `userver_target_generate_chaotic` CMake function now has
  a `LINK_TARGETS` parameter to provide targets with paths to chaotic serializers. For example with
  `LINK_TARGETS userver::postgresql` chaotic would find the
  userver/postgresql/include/userver/chaotic/io/userver/storages/postgres/time_point_tz.hpp header with
  @ref storages::postgres::TimePointTz converters.

* gRPC and Protobuf
  * Implemented retries. See @ref scripts/docs/en/userver/grpc/timeouts_retries.md for more info.
  * @ref ugrpc::client::ClientFactoryComponent now can be customized via `ssl-credentials-options`. Many thanks to
    [aklyuchev86](https://github.com/aklyuchev86) for the PR!
  * @ref decimal64::Decimal::FromStringPermissive() now understands format with exponent, which makes it compatible
    with Protobuf well-known Decimal types.

* Database drivers
  * Implemented @ref storages::redis::Client::GenericCommand() for execution of custom commands.
  * Redis now supports `sentinel_password` @ref storages::secdist::Secdist option for authorization on sentinels with
    password along with `password` authorization on shards. Many thanks to [Tikhon Sergienko](https://github.com/tysion)
    for the PR!
  * Implemented @ref storages::mongo::Collection::Distinct() command.
  * Implemented @ref storages::clickhouse::ParameterStore. Many thanks to [Alexey Medvedev](https://github.com/Reavolt)
    for the PR!
  * `update-period` for @ref components::Secdist is now set to `10s`. As a result all the databases now automatically
    recreate connections on connection data change in SecDist.
  * Implemented parsing of C++ `enum` into PostgreSQL text for `enums` with `Parse()` and `ToString()` functions. See
    @ref scripts/docs/en/userver/pg/user_types.md for more info.

* Optimizations
  * @ref concurrent::BackgroundTaskStorage was optimized with striped counters. Detaching a task now takes ~6% of
    the whole CPU usage of the detaching benchmark (before the optimizations it was ~46%).
  * @ref storages::Query now avoids dynamic initialization when constructed from @ref storages::Query::NameLiteral.
    As a result variables generated with CMake `userver_add_sql_library` function are now not affected by initialization
    order fiasco issues and can be safely used in any static or global variable. Dynamic memory allocations are
    now avoided for the above cases.
  * @ref utils::zstring_view now used for Kafka topic names avoiding temporary string constructions.

* Build
  * The oldest supported Python is now Python 3.9.
  * Protobuf 6.x.y is now supported. Thanks to [halfdarkangel](https://github.com/halfdarkangel) for the PR!
  * Supported by-component installation in CPack. See `USERVER_INSTALL_MULTIPACKAGE` CMake option for a way to enable
    the feature.
  * Optimized compilation time for the @ref userver/utils/periodic_task.hpp header by removing inclusion of 7
    heavy headers.
  * Improved build times by disabling modules scan. Many thanks to [Konstantin Goncharik](https://github.com/botanegg)
    for the PR!
  * Builds are now tested on Fedora.
  * Bumped Kafka version to 4.0 in CI checks. Many thanks to [MichaelKab](https://github.com/MichaelKab) for PR!

* Documentation and Diagnostics:
  * Version change widget was rewritten to avoid versions hard-code. Many thanks to
    [Lone-Marshal](https://github.com/Lone-Marshal) for the PR!
  * Added docs on @ref scripts/docs/en/userver/distro_maintainers.md roles.
  * Added more information on metadata usage into @ref scripts/docs/en/userver/grpc/grpc.md
  * `USERVER_GTEST_ENABLE_STACK_USAGE_MONITOR` environment variable name was changed
    to `USERVER_ENABLE_STACK_USAGE_MONITOR`. `USERVER_ENABLE_STACK_USAGE_MONITOR` is now usable not only in unit-tests,
    but in all the userver based applications and services.
  * Redis driver now does much more logging on cluster topology change to ease the cluster issues debugging.
  * Dynamic config pages with description are now generated from schemes.


### Release v2.11

* Added support for TLS in RedisCluster mode. Many thanks to [Danilkormilin](https://github.com/Danilkormilin) for the PR!
* Added ascetic web interface for the [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf).
* @ref server::handlers::HttpHandlerStatic now can serve static files from non-root URL paths. Many thanks to
  [Konstantin Goncharik](https://github.com/botanegg) for the PR!
* Added retry budget plugin for HTTP clients @ref clients::http::plugins::retry_budget::Component.
* @ref server::handlers::HttpHandlerStatic now has `directory-file` static config option that returns file in directory
  requests. It is set to "index.html" by default, so that `http://localhost/` requests return the `index.html`. Option
  `not-found-file` allows customizing 404 pages.
* Added @ref utils::move_only_function.
* Added seek functionality to @ref kafka::ConsumerScope. Many thanks to
  [Mikhail Romaneev](https://github.com/melonaerial) for the PR!
* Add a static config option `message_key_log_format` to @ref kafka::ConsumerComponent to log message key in hex.
  Many thanks to [Mikhail Romaneev](https://github.com/melonaerial) for the PR!
* @ref utils::statistics::HistogramView now provides total sum to comply with Prometheus format. Many thanks to
  [DmitriyH](https://github.com/DmitriyH) for the PR!
* Added @ref UTEST_P_DEATH and @ref UEXPECT_DEBUG_DEATH macro to `userver::utest`.
* Renamed `utils::NullTerminatedView` to @ref utils::zstring_view to match C++29 targeted proposal P3655R1.
* Added Get/Set batch size for @ref storages::mongo::Cursor. Many thanks to
  [Konstantin Goncharik](https://github.com/botanegg) for the PR!
* Added @ref engine::GetQueueSize(). Many thanks to [Emil Rakhimov](https://github.com/RakhimovEmil) for the PR!
* Fixed `std::int64_t` narrowing to `std::int32_t` in Kafka. Many thanks to
  [Mikhail Romaneev](https://github.com/melonaerial) for the PR!
* Chaotic now validates array type before parsing to `chaotic::Array`. Many thanks to
  [Konstantin Goncharik](https://github.com/botanegg) for the PR!
* Allow dynamic selection of response streaming. Many thanks to [Sergei Fedorov](https://github.com/zmij)
  for the PR!
* Added support for @ref storages::mongo::options::Hint for @ref storages::mongo::operations::Delete,
  @ref storages::mongo::bulk_ops::Update and @ref  storages::mongo::bulk_ops::Delete.
* @ref storages::Query now used in ClickHouse, MySQL and SQLite drivers, making it possible to directly use result of
  generation @ref scripts/docs/en/userver/sql_files.md "external SQL/YQL files". storages::Query::Statement() and
  storages::Query::GetName() are now deprecated and will be removed soon.

* gRPC:
  * gRPC clients now have @ref ugrpc::client::Reader, @ref ugrpc::client::Writer and @ref ugrpc::client::ReaderWriter
    names to match `ugrpc::server` names.
  * Fixed data race in bidirectional stream client.
  * Optimized clients and server for cases when logging for handler or client is disabled.
  * Multiple optimizations for the gRPC logging. Up to 150 times faster logging in edge cases.

* Other Optimizations:
  * @ref utils::zstring_view is now used throughout the userver to avoid temporary `std::string` constructions. Affected
    components include PostgreSQL driver, HTTP clients, Chaotic, universal and Kafka.
  * URL utils for schema, query and fragment extraction now have overloads that return `std::string_view` and avoid
    dynamic memory allocations.
  * Properly make date header for @ref scripts/docs/en/userver/libraries/s3api.md. This avoids rare cases of
    CPU-intensive thread blocking. Many thanks to [Daniil Shvalov](https://github.com/danilshvalov) for the PR.
  * Size of @ref storages::redis::ReplyData dropped down to 32 bytes from 64 bytes, leading to less memory usage for
    responses with long arrays.
  * Optimized hex logging in Kafka. Many thanks to [Mikhail Romaneev](https://github.com/melonaerial) for the PR!

* Documentation and Diagnostics:
  * [RealMedium sample](https://github.com/userver-framework/realmedium_sample) was modernized and cleaned up. Now it
    can be used as a sample. Many thanks to [Liiizak](https://github.com/Liiizak) for multiple PRs!
  * Updated build instructions. Many thanks to [h1laryz](https://github.com/h1laryz) for the PR!
  * Clarified @ref http::ContentType parsing errors.
  * More clarifications on @ref engine::Task interruptions for @ref userver_concurrency "concurrency primitives".
  * `tools/*` became samples and were moved into `samples/`. Tests were added.
  * "task_processor" and "fs_task_processor" static options now have proper defaults.
    See @ref scripts/docs/en/userver/task_processors_guide.md
  * Documented log sinks and formats more thoroughly. See @ref scripts/docs/en/userver/logging.md.
  * Simplified @ref storages::redis::Client `*scan` commands usage, added samples and more descriptions.
  * More docs and samples (and functions) for the userver/http/url.hpp.
  * ODBC driver foundation gained an improved error handling and connection pool. Many thanks to
    [Alexey](https://github.com/Olex1313) for the PR!
  * Added @ref scripts/docs/en/userver/grpc/server_middleware_implementation.md,
    @ref scripts/docs/en/userver/grpc/client_middleware_implementation.md and
    @ref scripts/docs/en/userver/tutorial/static_content.md documentation pages.
  * Updated @ref scripts/docs/en/userver/dynamic_config.md with info on how to use kill switches in
    @ref scripts/docs/en/userver/functional_testing.md.

* Build:
  * `CMAKE_CXX_STANDARD` was set to `20` by default. C++17 still supported.
  * Added preliminary CMake configure support on Windows. Many thanks to [Alex](https://github.com/leha-bot) for
    the PR.
  * Added cmake-format config and formatted the CMake files. Many thanks to [Dzmitry Ivaniuk](https://github.com/idzm)
    for the PR!
  * Added `with_redis_tls` flag for support Redis TLS in Conan. Many thanks to
    [Mikhail Romaneev](https://github.com/melonaerial) for the PR!
  * `userver_testsuite_add` CMake function now works is used in subdirectory of a project. Many thanks to
    [DmitriyH](https://github.com/DmitriyH) for the PR!


### Release v2.10

* Initial implementation of @ref scripts/docs/en/userver/sqlite/sqlite_driver.md. Many thanks to
  [Turulin Zakhar](https://github.com/zahartd) for the implementation, tests and for the documentation.
* GDB pretty printers now can list all the tasks via `utask list` and can apply commands to all or selected
  tasks. For example `utask apply all bt` prints the backtraces of all the tasks, `utask apply some_task_name bt`
  prints the backtrace of the task with name `some_task_name`. See
  @ref scripts/docs/en/userver/gdb_debugging.md for more info. Many thanks to
  [Maxim Belov](https://github.com/UNEXPECTEDsemicolon) for the brilliant implementation.
* Merged a foundation for the ODBC driver. Many thanks to [Alexey](https://github.com/Olex1313) for the PR!
* Redis driver now can ignore ping times to different instances to do a fair round-robin. See `consider_ping` field
  in storages::redis::CommandControl.
* Dropped gRPC `[(userver.field).secret = true];`. Use `[debug_redact = true];` instead.
* Statically assert that a destructor of an object in utils::FastPimpl is `noexcept`. Many thanks to
  [Шаблов Анатолий Владимирович](https://github.com/AnatoliiShablov) for the PR!

* Optimizations:
  * Reduced memory allocations while generating strings for ID in tracing::Span. About a 150ns speedup on average on
    tracing::Span construction.
  * Removed unused code and class members in Redis internals, reducing runtime memory usage and binary code size.
  * Replaced `std::unique_lock` with `std::lock_guard` where possible to simplify optimization work for the compiler.


### Release v2.9

* Logging now supports `fmt` formatting in macro `LOG_INFO("User {} logged in from {}", user_id, ip_address);` and
  lambda formatting. See @ref scripts/docs/en/userver/logging.md for more info.
* PostgreSQL driver now can disable all the statements logging via static config option `statement-log-mode`
* ClickHouse driver now supports doubles in the queries.
* YDB now can be used with GCC compiler, not only Clang. YDB still requires C++20 support
* components::Redis in sentinel mode now supports selection of database index via `database_index` in secdist config.
  Many thanks to [Tikhon Sergienko](https://github.com/tysion) for the PR!
* PostgreSQL cache (shadow replicas) traits now support `kOrderBy`. With `DISTINCT ON` expression in `kQuery` it allows
  to store only slices of data. Many thanks to [Dmitry Kopturov](https://github.com/IsThisLoss) for the PR!
* Added a `userver-create-service` script for @ref quick_start_for_beginners "creation of a new service". Github
  service templates are now deprecated.
* Notify on state change when it actually happens in Redis Standalone. Many thanks to
  [Nikolay Pervushin](https://github.com/Greenvi4) for the PR!
* Modernized testsuite code, including removal of `event_loop` usages and usage of asyncio-socket.
* Hidden `thread_id` and `task_id` for `INFO`+ logger levels to make the logs shorter.
* Stack usage monitor now could be disabled in test via `USERVER_GTEST_ENABLE_STACK_USAGE_MONITOR=0`
  environment variable.

* gRPC
  * Added support for `debug_redact` and added a recommendation to use `[debug_redact = true];` instead
    of `[(userver.field).secret = true];`
  * Significant speed up logging of large requests and response messages. Note that request/response logs are by
    default enabled with global `DEBUG` logging level; the behavior could be overridden by middlewares static configs
    of ugrpc::client::middlewares::log::Component and ugrpc::server::middlewares::log::Component.
  * Server and client logging tags are now consistent.
  * More improvements, docs and samples for middlewares. See @ref scripts/docs/en/userver/grpc/grpc.md for more info.

* Build:
  * Fixed floating point escaping compilation in ClickHouse driver. Many thanks to
    [Ksenia-C](https://github.com/Ksenia-C) for the PR!
  * Fix compilation errors for curl 8.13. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Fix compilation errors for fmt 11. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PR!
  * Added `USERVER_ENABLE_DEBUG_INFO_COMPRESSION` build option, that was later changed to
    `USERVER_DEBUG_INFO_COMPRESSION`. Many thanks to [Konstantin Goncharik](https://github.com/botanegg) for the PRs!
    Compression detection algorithm was improved to check the linker compression support.
  * Added Debian-12 dependencies and build instructions.
  * Export only files that are not ignored by git for Conan sources. Many thanks to
    [c0rt](https://github.com/c0rt) for the PR!

* Documentation and Diagnostics:
  * Added @ref scripts/docs/en/userver/codegen_overview.md, @ref scripts/docs/en/userver/sql_files.md,
    @ref scripts/docs/en/userver/stack.md documentation.
  * Fixed a typo in MacOS build dependencies. Thanks to [Alexey](https://github.com/Olex1313) for the PR!
  * Fixed formatting of CancellableSemaphore::GetCapacity(). Thanks to
    [Sergey Prikhodko](https://github.com/mhth-fn) for the PR!
  * Fixed uncountable amount of issues with doxygen markup.
  * Structures and parsers for userver dynamic configs are now generated by chaotic from schemas.
  * More docs for utest::HttpServerMock.
  * Reworked Kafka metrics and added @ref scripts/docs/en/userver/kafka.md "docs about them".
  * Added docs and samples for Eval* functions of Redis driver.


### Release v2.8

* `sharding_strategy` of the components::Redis now supports `RedisStandalone` configuration, that may be useful for
  tests or unimportant caches. Many thanks to [Aleksey Ignatiev](https://github.com/ae-ignatiev) for the PR!
* kafka::Producer now has the API for sending Kafka headers. Many thanks
  to [Mikhail Romaneev](https://github.com/melonaerial) for the PR and to [Fedor Lobanov](https://github.com/fslobanov)
  for the fix!
* kafka::ConsumerScope now has the API for receiving Kafka headers.
* Add span events to OpenTelemetry via tracing::Span::AddEvent(). Many thanks
  to [Dudnik Pavel](https://github.com/nepridumalnik) for the PR.
* Added logging::JsonString to explicitly describe in type system that a string contains loggable JSON. Many thanks
  to [akhoroshev](https://github.com/akhoroshev) for the PR!
* Deadline Propagation for PostgreSQL is now enabled by default and can be controlled via `deadline-propagation-enabled`
  static option of the components::Postgres.
* Testsuite now supports @ref uservice_oneshot "`@@pytest.mark.uservice_oneshot`".
* utils::regex now always uses a faster and safer Re2 instead of boost::regex.
* Dynamic config `USERVER_HANDLER_STREAM_API_ENABLED` is not used any more.
* server::handlers::HttpHandlerStatic now has a `expires` static config option.
* kafka::ProducerComponent and kafka::ConsumerComponent now support 'SASL_PLAINTEXT'
  security protocol. Many thanks to [Mikhail Romaneev](https://github.com/melonaerial) for the PR!
* Implemented OneOf discriminator mapping to integer and generation of fmt::formatters for enums in chaotic.
* Load `kRoundRobin` load distribution in PostgreSQL is now uniform

* gRPC
  * Retries are now supported and controlled via dynamic config. See ugrpc::client::Qos for more info.
  * Clients and servers now use the same configuration approach for middlewares, allowing granular overrides
    of settings.

* Optimizations
  * Postgres driver now uses `moodycamel` queue instead of boost::lockfree. Up to 2 times faster retrieval of
    connection from pool.
  * utils::TrivialBiMap is used in more cases, leading to faster runtime search and minor decrease in binaries size.
  * Multiple std::string constants were replaced with `constinit` types, leading to faster startup times and
    smaller binaries.
  * Avoid `url` copies in clients::http::Request

* Documentation and Diagnostics:
  * Fixed typo at @ref scripts/docs/en/userver/build/dependencies.md. Many thanks
    to [Konstantin Goncharik](https://github.com/botanegg) for the PR.
  * More docs on @ref scripts/docs/en/userver/sql_files.md
  * Improved parse failure messages for @ref scripts/docs/en/userver/dynamic_config.md
  * Improved diagnostics for corrupted tracing::Span
  * Erroneous attempt to log function address is now captured at build time.

* Testing
  * Easy grpc mock registration in testsuite.
  * Daemon-scoped fixtures implemented via `@pytest.mark.uservice_oneshot`.

* Build
  * Debug symbols of userver libraries are now compressed with `zstd` if the toolset supports it, leading to
    smaller binaries size.
  * Docker images now use `zstd` compression too.
  * Dropped CI testing on Ubuntu 20.04 which lifetime almost ended.
  * Improved build type matching for installed userver. Many thanks
    to [Aleksey Ignatiev](https://github.com/ae-ignatiev) for the PR!


### Release v2.7

* Logging in JSON format was implemented. See static option `format` at components::Logging.
* utils::regex now uses `Re2` under the hood, leading to at least x2 faster regular expression matching and guaranteed
  absence of backtracking. Updating is highly recommended.
* Mongo connection state checking algorithms was adjusted to work well on small RPS.
* Conan packages now support all the userver features. Conan package build now reuses the CMake install targets and
  CMake config files.
* Full feature support for MacOS, including testing and Conan package build and usage on that platform.
* Added support for TLS certificate chains. See `tls.cert` static option at components::Server. Many thanks to
  [aklyuchev](https://github.com/aklyuchev) for the PR!
* Chaotic exceptions now do not depend on JSON. Thanks to [Artyom](https://github.com/Lookingforcommit) for the PR!

* gRPC
  * Out-the-box cache dump support for Protobuf messages.
    @ref dump_serialization_guide "Implementing serialization (Write / Read)" for more info.
  * Removed deprecated `*Sync` methods.

* Optimizations
  * Speed up configuration reads on creating new PostgreSQL connections.
  * utils::PeriodicTask now calls RCU Read two times less on each iteration.

* Build
  * Fixed build with `USERVER_FEATURE_JEMALLOC=ON`. Many thanks to [Aleksey Ignatiev](https://github.com/ae-ignatiev)
    for the PR!
  * Service templates [service_template](https://github.com/userver-framework/service_template),
    [pg_service_template](https://github.com/userver-framework/pg_service_template),
    [pg_grpc_service_template](https://github.com/userver-framework/pg_grpc_service_template),
    [mongo_grpc_service_template](https://github.com/userver-framework/mongo_grpc_service_template) now use
    @ref service_templates_presets "cmake presets" and @ref devcontainers "devcontainers" for out-of-the-box support
    of VSCode and Clion IDEs.
  * Started the work on Ubuntu 24.04 images.
  * Added `ubuntu-22.04-userver-pg-dev` image with all the tools for development. Planning to switch to Ubuntu-24.04 and
    leave only 2 containers: with build dependencies to build userver, and with prebuild userver.
  * Added missing fmt11 headers. Thanks to [Pavel Sidorovich](https://github.com/RayeS2070) for the PR!
  * Added `USERVER_USE_STATIC_LIBS` to link third-party libraries statically.
  * Support `pacman` epoch in CMake version detection. Many thanks to [Konstantin Goncharik](https://github.com/botanegg)
    for the PR.

* Documentation
  * Significant update of the @ref scripts/docs/en/userver/build/build.md
  * More docs for tracing::Span::SetLogLevel() and tracing::Span::SetLocalLogLevel()
  * Fixed secdist example at components::Mongo. Thank to [Nikita Puteev](https://github.com/Malfak) for the PR!
  * Highlight the functionality of formats::common::Item in each supported format.
  * Add info about full static linkage. Thanks to [Nikita](https://github.com/root-kidik) for the PR!
  * Better `runtests` documentation at @ref scripts/docs/en/userver/functional_testing.md
  * Documentation and samples for storages::postgres::io::Codegen{}.


### Release v2.6

* storages::secdist::Secdist is now automatically reloaded for Mongo, Redis and PostgreSQL databases if the secdist file
  was changed. Now changing the connection parameters in file does not require service restart.
* Public parts of the Redis driver were moved out from `impl/` directory and placed into `storages::redis::` namespace.
  If you were relying on the old paths, see `./scripts/migrate_from_legacy_redis_ns.sh` script to ease migration.
* Shortened testsuite logs were made more functional by providing HTTP URL info.
* Removed old gRPC interface for server handlers as was promised in previous release notes.
* gRPC client interfaces were changed to be more user friendly. For example, for `HelloWorld` method in protobuf we
  generate the old `HelloWorld` function along with the new `AsyncHelloWorld` and `SyncHelloWorld` functions.
  `AsyncHelloWorld` returns a `ugrpc::client::ResponseFuture` that can be used to retrieve the request result later
  in code. `SyncHelloWorld` retrieves the response from the future and returns the response itself.
  Consider replacing:
  * `HelloWorld(x).Finish()` with `SyncHelloWorld(x)`
  * `auto res = HelloWorld(x); /* a lot of code */; res.Finish();` with
    `auto res = AsyncHelloWorld(x); /* a lot of code*/; res.Get();`
  In next release we will remove the old `HelloWorld` and will rename `SyncHelloWorld` into `HelloWorld`.
* Added @ref scripts/docs/en/userver/libraries/easy.md. Now the service can be created in a few code lines:
  ```cpp
  int main(int argc, char* argv[]) {
    easy::HttpWith<>(argc, argv)
        .DefaultContentType(http::content_type::kTextPlain)
        .Route("/hello", [](const server::http::HttpRequest& /*req*/) {
            return "Hello world";  // Just return the string as a response body
        });
  }
  ```
* Added `userver_embed_file` CMake function to embed files into the binary.
  See @ref scripts/docs/en/userver/tutorial/hello_service.md for an example.
* Queries now @ref scripts/docs/en/userver/sql_files.md "can be moved to a separate files".
* Added graceful shutdown functionality. See `graceful_shutdown_interval` in components::ManagerControllerComponent.
* server::http::HttpRequestBuilder now can be used to create server::http::HttpRequest in unit tests.
* Kafka driver now has kafka::ConsumerScope::GetPartitionIds() and kafka::ConsumerScope::GetOffsetRange() functions.
  Many thanks to [Kirill](https://github.com/KVolodin) for the PR!
* @ref opentelemetry "OpenTelemetry" now sends `span_kind` information.

* Added `user`, `password`, and `secure_connection_cert` parameters support for the YDB secdist. Thanks to
  [Попов Алексей](https://github.com/popov-aa) for the PR!
* @ref POSTGRES_TOPOLOGY_SETTINGS now has `disabled_replicas` option to disable some of the replicas.
* Fixed Kafka logs being written into STDERR in edge cases. Thanks to [Dudnik Pavel](https://github.com/nepridumalnik)
  for the PR!
* Added unbounded queue variants concurrent::UnboundedNonFifoMpscQueue, concurrent::UnboundedSpmcQueue,
  and concurrent::UnboundedSpscQueue. Those queues are usually x2 faster than the bounded variants.
* `GT` and `LT` flags support in Redis `ZADD`. Thanks to [Nikolay Pervushin](https://github.com/Greenvi4) for the PR!
* Reduced condition in OTLP, thanks to [Dudnik Pavel](https://github.com/nepridumalnik).

* Build:
  * Simplified Profile Guided Optimization (PGO) gathering and usage due to new `USERVER_PGO_GENERATE` and
    `USERVER_PGO_USE` CMake options. See @ref scripts/docs/en/userver/build/build.md for more info.
  * MacOS now can build the userver as a Conan package.
  * Build flags were reorganized to use a new `USERVER_BUILD_ALL_LIBRARIES` CMake option.
    See @ref scripts/docs/en/userver/build/options.md for more info.
  * Source directory now can contain spaces.
  * Correctly set grpc-reflection found flag. Thanks to [Nikita](https://github.com/rtkid-nik) for the PR!
  * Fixed `USERVER_CHAOTIC_FORMAT` option for CMake build. Thanks to [Konstantin Goncharik](https://github.com/botanegg)
    for the PR.
  * Optimized reconfiguration in CMake giving up to 60% time save (6-20 seconds).

* Documentation and diagnostics:
  * More information on Mongo heartbeat in logs.
  * Added docs about tag name of tracing::ScopeTime.
  * Improved PostgreSQL diagnostic messages for server response parsing errors due to C++ and DB types mismatch.
  * Better samples and docs for utils::statistics::Writer.
  * Added direct database access to testsuite samples.
  * Updated the @ref concurrent_queues "Concurrent Queues" docs.
  * Log formats message was amended. Thanks to [tkhanipov](https://github.com/tkhanipov) for the PR!


### Release v2.5

* Added @ref scripts/docs/en/userver/libraries/s3api.md "S3 API client s3api::Client". Many thanks to
  [v-for-vandal](https://github.com/v-for-vandal) for the work!
* Added @ref scripts/docs/en/userver/libraries/grpc-reflection.md "gRPC reflection library". Many thanks to
  [v-for-vandal](https://github.com/v-for-vandal) for the work!
* Added @ref kill_switches "Kill Switch" functionality. Many thanks to
  [Aksenov Anton](https://github.com/Dangerio) for the work!
* @ref scripts/docs/en/userver/congestion_control.md "Congestion Control" turned on by default.
* Initial work towards embedding GDB pretty-printers to userver binaries.
* Mongo now has the full functionality for diagnostics out-of-the box, without mongo-c library patches.
* Simplified contributing by removing the annoying bot that checks for explicit agreement to CLA. Creating an issue or
  sending a PR already means agreement with CLA. Added notes to PR and Issue creation to highlight that.
* Basic support for HTTP/2 body streaming.
* Kafka support in testsuite implemented. See Functional tests section at
  @ref scripts/docs/en/userver/tutorial/kafka_service.md tutorial.

* gRPC:
  * Safe new interface for gRPC server handlers. **Old interface will be removed in next release.**
  * Added support for TLS in gRPC.
  * Added ugrpc::server::middlewares::field_mask::Component for masking and trimming messages. Many thanks to
    [TTPO100AJIEX](https://github.com/TTPO100AJIEX) for the work!
  * gRPC clients now allow configuring channels count for particular methods via `dedicated-channel-counts` static
    config option.

* Optimizations:
  * concurrent::MpscQueue was optimized, leading to x2-x3 better performance.
  * rcu::Variable deleter now can be chosen at compile time, leading to smaller size of rcu::Variable if no asynchronous
    deletion required.
  * Multiple optimizations for gRPC logging and message visitations via ugrpc::VisitFieldsRecursive().  Many thanks to
    [TTPO100AJIEX](https://github.com/TTPO100AJIEX) for the work!

* Build:
  * Added `userver_module()` CMake function to simplify configuration of new drivers that are being added to userver.
  * Added missing `fmt/ranges.h` includes. Thanks to [Vasilii Kuziakin](https://github.com/Basiliuss) and to
    [SidorovichPavel](https://github.com/SidorovichPavel) for the PRs!
  * Proper use of `PROTOBUF_PROTOC` in CMake. Thanks to [Nikita](https://github.com/rtkid-nik) for the PR!
  * Added support for builds in paths that contain whitespaces and other special symbols.
  * Added CI build tests for Ubuntu 24.04 and MacOS.
  * Switched to Conan v2. Many thanks to [Anton](https://github.com/xakod) for the PR! Also use modern versions of
    third party libraries in Conan.

* Documentation and diagnostics:
  * A whole new build dedicated section was added to the docs instead of the old "Configure, Build and Install" page.
  * Improved schemes validation messages, including config validation messages because no schema is written.
  * Disambiguated diagnostic messages for component system.
  * Better log messages for the dist locks.
  * Better docs for gRPC middlewares and gRPC logs at @ref scripts/docs/en/userver/grpc/grpc.md.
  * Added topology and heartbeats logs and metrics for Mongo.
  * Clarified docs on PostgreSQL data types with timezones. See @ref scripts/docs/en/userver/pg/types.md.
  * Added @ref scripts/docs/en/userver/tutorial/kafka_service.md tutorial.
  * @ref scripts/docs/en/userver/log_level_running_service.md, @ref scripts/docs/en/userver/congestion_control.md
    documentation rewrite.


### Release v2.4

* Added @ref USERVER_LOG_REQUEST_HEADERS_WHITELIST to control the HTTP headers
  to log.
* OpenTelemetry protocol (OTLP) now can optionally do only logging or only
  tracing. Thanks to [TertiumOrganum1](https://github.com/TertiumOrganum1) for
  the PR!
* The framework now accepts OTLP headers for tracing by default and puts those
  headers for new requests.
* PostgreSQL span names are now a little bit more informative. Thanks to
  [TertiumOrganum1](https://github.com/TertiumOrganum1) for the PR!
* Kafka now has a `client.id` static option. Many thanks to
  [Nikolay Pervushin](https://github.com/Greenvi4) for the PR.
* PostgreSQL type errors become more informative. Thanks to
  [farmovit](https://github.com/farmovit) for the report!

* Optimizations:
  * HTTP/2 server implementation now does not copy data to send, saving CPU
    and RAM.
  * HTTP/2 now relies on open-addressing unordered map from nghttp2, leading to
    faster stream lookup.
  * Kafka consumer now does not block a task processor thread, allowing
    multiple consumers to share the same OS thread. Consume cycle now can be
    treated as an asynchronous non-blocking event loop.
  * Kafka producer delivery acknowledgments processing is now done in parallel,
    leading to better scalability. Also it does not block the OS thread when
    waiting for new delivery acknowledgments.
  * Internals of all the Sockets became smaller in size, saving some RAM.

* gRPC:
  * gRPC in testsuite now automatically calls
    @ref pytest_userver.client.Client.update_server_state update_server_state.
    The behavior now matches HTTP.
  * gRPC server now supports unix-sockets via `unix-socket-path` static config
    option.
  * gRPC clients now log requests/responses via the
    ugrpc::client::middlewares::log::Component middleware. Improved gRPC
    client and server metrics.
  * New component ugrpc::client::CommonComponent with common options for all the
    gRPC clients.

* Build, Install and CI:
  * OTLP build is now supported in Conan. Thanks to
    [Amina Ramazanova](https://github.com/konataa) for the PR!
  * Chaotic now exposes less headers, leading to faster build times.
  * Fixed compilation on modern Boost.UUID. Thanks to
    [Alexander Botev](https://github.com/MrSteelRat) for the PR!
  * Added `dependabot` to CI and updated the dependencies. Thanks to
    [Dzmitry Ivaniuk](https://github.com/idzm) for the PR!
  * Added missing `#include`. Thanks to [Nikita](https://github.com/rtkid-nik)
    for the PR!
  * Removed outdated defines in the core. Thanks to
    [Sergey Kazmin](https://github.com/yerseg) for the PR!
  * Install now does not put third party headers into the top level include
    directory. Multiple unused files are now not installed.
  * Started the work to enable builds in directories with whitespace in names.

* Documentation:
  * More docs for gRPC middlewares at @ref scripts/docs/en/userver/grpc/grpc.md
    and @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md.
  * More docs for otlp::LoggerComponent. Thanks to
    [TertiumOrganum1](https://github.com/TertiumOrganum1) for the PR!
  * Set proper `Content-Type` in samples.


### Release v2.3

* Initial HTTP 2.0 server support is now implemented. Use
  `handler-defaults.http_version` static config option of components::Server to
  enable.
* Logger for OpenTelemetry protocol was implemented. Could be enabled via
  `USERVER_FEATURE_OTLP` CMake option. See @ref opentelemetry "the docs" for
  more info.
* Client address in handler now could be retrieved via
  server::http::HttpRequest::GetRemoteAddress(). Many thanks to
  [Daniil Shvalov](https://github.com/danilshvalov) for the PR.
* The scheduler implementation now could be adjusted for each task_processor
  via `task-processor-queue` static option. A more efficient
  `work-stealing-task-queue` was introduced. Many thanks to
  [Egor Bocharov](https://github.com/egor-bystepdev) for the PR!
* Added storages::postgres::TimePointWithoutTz for more explicit declaration of
  intent. Direct work with std::chrono::system_clock is now deprecated in
  PostgreSQL driver.
* Validation of static config schemas now understands `minItems` and `maxItems`
  for arrays. Many thanks to [eparoshin](https://github.com/eparoshin) for the
  PR.
* Websockets now have case insensitive check of headers. Thanks to
  [Alexander Enaldiev](https://github.com/Turim) for the PR!
* Added engine::io::Socket::ReadNoblock() function to check if there's a
  pending data and read it if any. server::websocket::WebSocketConnection now
  has a TryRecv() function to receive a message if its first bytes already came.
  Thanks to [Alexander Enaldiev](https://github.com/Turim) for the PR!
* `#env`, `#file` and `#fallback` now could be used in `config_vars` file.
  See yaml_config::YamlConfig for more info. Thanks to
  [Artyom Samuylik](https://github.com/Matrix-On) for the PR.
* gRPC
  * Sensitive data now could be hidden in logs via applying a
    `[(userver.field).secret = true]` option to a protobuf field in schema.
  * Generic server now could be implemented via
    ugrpc::server::GenericServiceBase. Generic client
    ugrpc::client::GenericClient was also implemented. The functionality of
    generic client/server is useful for writing gRPC proxies.
  * gRPC server now shows aggregated `grpc.server.total` metrics
  * More samples and docs.
* Optimizations:
  * IO events are now uniformly distributed between ev threads. This leads
    to better performance on high loads in default configurations. Even number
    of ev threads now works as good as odd number of threads.
  * IO watchers now always start asynchronously, leading to x2 less CPU
    consumption for each start+stop operation. As a result ev threads of HTTP
    client and Redis driver now use less CPU.
  * Timer events with reachable deadlines now are deferred if that does not
    affect latencies. This gives ~5% RPS improvement for `service_template`.
* Build
  * `Find*.cmake` files are not generated any more, leading to simpler code base
    and faster configure times.
  * Fixed incorrect handling of dots in chaotic paths. Thanks to
    [Alexander Chernov](https://github.com/blackav) for the PR!
  * MacOS build options are now part of the CMake files, leading to less
    boilerplate while compiling for that platform. Many thanks to
    [Daniil Shvalov](https://github.com/danilshvalov) for the PR.
  * Kafka driver is now enabled in Conan. Many thanks to
    [Aleksandr Gusev](https://github.com/ALumad) for the PR.
  * Conan related build fixes. Thanks to [Alex](https://github.com/leha-bot) for
    the PR.


### Release v2.2

* Added @ref scripts/docs/en/userver/chaotic.md "codegen parsers and serializers by JSON schema"
* Improved the ability to unit test of gRPC clients and servers, Redis and
  Mongo databases, logs. Samples and docs were improved.
* Implemented feedback forms and likes/dislikes for each documentation page.
  **Feedback would be appreciated!**
  Many thanks to [Fedor Alekseev](https://github.com/atlz253) for the PR and to
  [MariaGrinchenko](https://github.com/MariaGrinchenko) for the buttons design!
* Added @ref scripts/docs/en/userver/ydb.md "docs on YDB".
* Mobile header view and docs layout was improved. Many thanks to
  [Fedor Alekseev](https://github.com/atlz253) for the PRs.
* engine::subprocess::ProcessStarter::Exec now can lookup binaries via
  `PATH` variable.
* Fixed gRPC generation for nested namespaces with repetitions. Many thanks to
  [nfrmtk](https://github.com/nfrmtk) for the PR!
* Handle both websocket and plain HTTP requests for the same path. Many thanks
  to [Hovard Smith](https://github.com/w15eacre) for the PR!
* Support setting client + CA certs in RabbitMQ. Many thanks to
  [Alexey Dyumin](https://github.com/dyumin) for  the PR!
* yaml_config::YamlConfig now can read files via `#file`. Now the static
  config of the service could refer to other files.
* Added support of bit operations to Redis.
* PostgreSQL driver now works with AWS Aurora.
* Added quick start for beginners to @ref scripts/docs/en/userver/build/build.md.
  Many thanks to [Fedor Alekseev](https://github.com/atlz253) for the PR.
* Improved path to sources trimming for Conan builds. Many thanks to
  [Kirill](https://github.com/KVolodin) for the PR!
* Multiple minor improvements to build, packaging, docs and testing.

### Release v2.1 (May 2024)

* Coroutines stack usage is now shown in the
  `engine.coro-pool.stack-usage.max-usage-percent` metric. Improved
  stack-overflow diagnostics.
* HTTP server and HTTP client now support ZSTD decompression. Thanks
  to [Илья Оплачкин](https://github.com/IoplachkinI)
  and [VScdr](https://github.com/VS-CDR) for the PR!
* Added redis::MakeBulkHedgedRedisRequestAsync() and
  redis::MakeBulkHedgedRedisRequest().
* OpenTelemetry parent span-id is now passed through AMQP headers along with
  trace-id. Thanks to [TertiumOrganum1](https://github.com/TertiumOrganum1) for
  the PR!
* ugrpc::server::MiddlewareBase now has CallRequestHook and CallResponseHook
  for intercepting requests and responses.
* components::LoggableComponentBase was renamed to components::ComponentBase.
  components::RawComponentBase was published.
* Multiple improvements for logging in testsuite.
* gRPC metrics are now not written for methods that were not used at runtime.
* Mongo pools now can be adjusted at runtime via dynamic config
  @ref MONGO_CONNECTION_POOL_SETTINGS. Congestion Control for individual Mongo
  databases now could be controlled via
  @ref MONGO_CONGESTION_CONTROL_DATABASES_SETTINGS. Congestion Control is now
  enabled by default.
* Reduced contention in coro::Pool and added some tests and benchmarks. Many
  thanks to [Egor Bocharov](https://github.com/egor-bystepdev) for the PRs!
* Added urabbitmq::ConsumerComponentBase::Process() accepting the whole
  urabbitmq::ConsumedMessage. Thanks to
  [TertiumOrganum1](https://github.com/TertiumOrganum1) for the PR!
* `human_logs.py` now supports more options and has more examples and docs
  embedded. Thanks to
  [TertiumOrganum1](https://github.com/TertiumOrganum1) for the PR!
* server::http::HttpStatus and client::http::Status are now aliases to
  http::StatusCode. Many thanks to
  [SidorovichPavel](https://github.com/SidorovichPavel) for the PR!

* Docs and build:
  * `find_package(userver)` now implicitly calls `userver_setup_environment()`,
    includes all the helper CMake userver scripts, making the configuration simpler.
    Added diagnostics and fix-it hints for some of the CMake missuses.
  * In docs `Ctrl+k` hotkey now focuses on `Search` input. Many thanks to
    [Fedor Alekseev](https://github.com/atlz253) for the PR!
  * ODR-violations are now avoided if the userver is built with different standard
    version than the service.
  * Each sample is now usable as a root project.
  * Each driver now has a @ref QUALITY_TIERS "Quality Tier".
  * Fixed minimal version requirements for Pythons gRPC modules. Thanks to
    [Nikita](https://github.com/root-kidik) for the PR!
  * Reduced build times by avoiding inclusion of heavy headers.
  * Added an example on PostgreSQL `bytea` usage. Thanks to
  [TertiumOrganum1](https://github.com/TertiumOrganum1) for the PR!
  * Multiple improvements for docs, build and CI scripts.

### Release v2.0

Big new features since the v1.0.0:

* Simplified dynamic configs and embedded defaults into the code.
* Added PostgreSQL connection pools auto-configuration.
* Added YDB driver and basic Kafka driver.
* LISTEN/NOTIFY support for PostgreSQL
* New landing page for the website
* Significantly reduced network data transmission for PostgreSQL
* Supported `install` in CMake and CPack packaging.
* Implemented middlewares for HTTP server, most of the HTTP server functionality
  was moved to middlewares.
* Improved documentation, added more samples and descriptions.
* Numerous optimizations and build improvements.

Detailed descriptions could be found below.

Binary Ubuntu 22.04 amd64 package could be found at
[userver Releases](https://github.com/userver-framework/userver/releases).


### May 2024 (v2.0-rc)

* Kafka driver is now documented, compiles and works. Thanks to
  [Fedor](https://github.com/fdr896) for the work!
* Added utils::numeric_cast for safe casting of integers of different width.
* The userver framework is now
  [available at Yandex Cloud Marketplace](https://yandex.cloud/en/marketplace/products/yc/userver).
* YDB driver now can be built on modern Clang compilers in C++20 and above
  modes.
* Redis now allows you to subscribe to master instances.
* Improved logging of failures in testsuite.

* Optimizations:
  * rcu::Variable was optimized to use asymmetric fences and
    concurrent::StripedCounter. x1000 performance improvement for some edge
    cases, x3 improvement for the generic use case.
  * Internal `TaskCounter` now uses concurrent::StripedCounter to reduce
    contention on atomics on each async task construction and destruction.
  * Adjusted the default spinning count in scheduler to better fit modern
    hardware and typical loads.
  * ~5% faster tasks queue overload detection logic. Many thanks to
    [Egor Bocharov](https://github.com/egor-bystepdev) for the PR!

* Docs, tests and build
  * Fixed build of utils/rand.hpp related source files. Thanks to
    [Nikita](https://github.com/root-kidik) for the PR!
  * Improved logic of Telegram support chats URL selection. Many thanks to
    [Mingaripov Niyaz](https://github.com/mnink275) for the PR!
  * Fixed multiple `@snippet` links in docs. Many thanks to
    [Mingaripov Niyaz](https://github.com/mnink275) for the PR!
  * Fixed flaky `ThreadLocal.SafeThreadLocalWorks` test. Many thanks to
    [Egor Bocharov](https://github.com/egor-bystepdev) for the PR!


### Older releases

* @ref scripts/docs/en/userver/roadmap_and_changelog_v1.md "v1 and before"

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/development/releases.md | @ref scripts/docs/en/userver/distro_maintainers.md ⇨
@htmlonly </div> @endhtmlonly

