# Roadmap and Changelog

We keep an eye on all the issues and feature requests at the
[github.com/userver-framework](https://github.com/userver-framework), at the
[English-speaking](https://t.me/userver_en) and
[Russian-speaking](https://t.me/userver_ru) Telegram support chats. All the
good ideas are discussed, big and important ones go to the Roadmap. We also
have our in-house feature requests, those could be also found in Roadmap.

Important or interesting features go to the changelog as they get implemented.
Note that there's also a @ref md_en_userver_security_changelog.

Changelog news also go to the
[userver framework news channel](https://t.me/userver_news).


## Roadmap


### Plans for the first release

* Add web interface to the [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf)
  * ✓ Add component to serve static pages
* Migrate our internal CI to the github
* Migrate to upstream versions of formatters
* Improve documentation
  * ✓ Improve @ref md_en_userver_framework_comparison
  * ✓ Add TCP acceptor sample
  * ✓ Add gRPC testsuite mock sample
  * ✓ Add reference sections for the Python fixtures
  * Add HTTP authentication sample
* Improve experience with metrics
  * ✓ Add Prometheus metrics format
  * ✓ Add Graphite metrics format
  * Provide a modern simple interface to write metrics
* Add chaos tests for drivers
  * ✓ Chaos proxy implemented
* Enable PostgreSQL pipelining
* Implement and enable Deadline Propagation
* Implement streaming API for the HTTP
* Add basic Kafka driver.


## Changelog

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
* Added gRPC testsuite mock sample at @ref md_en_userver_tutorial_grpc_service.
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
  @ref md_en_userver_service_monitor for details.
* Initial support for chaos testing was added, see
  @ref md_en_userver_chaos_testing for more info.
* Generic Escape implementation for ranges was added to clickhouse, thanks to
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
* gRPC mockserver support (docs will be updated soon at
  @ref md_en_userver_tutorial_grpc_service).
* gRPC now provides an efficient API for async execution of requests without
  additional `utils::Async` invocations.
* Build fixes for older platforms, thanks to
  [Yuri Bogomolov](https://github.com/ybogo) for the PR.
* components::Logging now can output logs to UNIX sockets.
* Now the "help wanted" issues at github have additional tags "good first issue"
  and "big", to help you to choose between a good starting issue and a big
  feature. See @ref md_en_userver_development_releases for more info.


### Beta (September 2022)

* [Ivan Trofimov](https://github.com/itrofimow) implemented the RabbitMQ driver.
* Added navigation to the next and previous page in docs, thanks to multiple
  feature requests in [Telegram support chat](https://t.me/userver_ru).
* Improved Task::Detach docs and added recomendation to use
  concurrent::BackgroundTaskStorage instead, thanks to
  [Ivan Trofimov](https://github.com/itrofimow) for the bugreport.
* Added `start-` targets for the samples, to simplify experimenting with them.
* Docs now support dark theme out ot the box.
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
  @ref md_en_userver_tutorial_tcp_service and @ref md_en_userver_tutorial_tcp_full,
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


### Pre anounce (May-Jul 2022)

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
  see @ref md_en_userver_component_system "the documentation".
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
* Docker suport: [base image for developement](https://github.com/userver-framework/docker-userver-build-base/pkgs/container/docker-userver-build-base),
  docker-compose.yaml for the userver with build and test targets.
  See @ref md_en_userver_tutorial_build
* Docs improved: removed internal links; added
  @ref md_en_userver_framework_comparison,
  @ref md_en_userver_supported_platforms, @ref md_en_userver_beta_state,
  @ref md_en_userver_security_changelog,
  @ref md_en_userver_profile_context_switches,
  @ref md_en_userver_driver_guide,
  @md_en_userver_task_processors_guide,
  @ref md_en_userver_os_signals and @ref md_en_userver_roadmap_and_changelog.
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
⇦ @ref md_en_userver_beta_state | @ref md_en_userver_tutorial_hello_service ⇨
@htmlonly </div> @endhtmlonly

