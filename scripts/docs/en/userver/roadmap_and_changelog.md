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

* Add component to serve static pages
  * Add web interface to the [uservice-dynconf](https://github.com/userver-framework/uservice-dynconf)
* Migrate our internal CI to the github
* Migrate to upstream versions of formatters
* Improve documentation
* Add a TCP acceptor sample
* Add Prometheus metrics format


## Changelog

### Beta (since end of May 2022 to public anouncement)
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

