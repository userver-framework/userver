## The C++ Asynchronous Framework

🐙 **userver** is the modern open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.

The framework solves the problem of efficient I/O interactions transparently for
the developers. As a result, with the framework you get straightforward source code,
avoid CPU-consuming context switches from OS, efficiently
utilize the CPU with a small amount of execution threads.


## Community and Telegram Support chats

Telegram support chats: [English-speaking](https://t.me/userver_en) |
[Russian-speaking](https://t.me/userver_ru) | [News channel](https://t.me/userver_news).


## Source codes and service templates at github

Ready-to-use services templates and the source codes of the framework itself
are available at the
[userver-framework at github](https://github.com/userver-framework/).


## Introduction
* @ref scripts/docs/en/userver/intro_io_bound_coro.md
* @ref scripts/docs/en/userver/intro.md
* @ref scripts/docs/en/userver/framework_comparison.md
* @ref scripts/docs/en/userver/supported_platforms.md
* @ref scripts/docs/en/userver/deploy_env.md
* @ref scripts/docs/en/userver/development/releases.md
* @ref scripts/docs/en/userver/roadmap_and_changelog.md
* @ref scripts/docs/en/userver/faq.md


@anchor Install
## Install

* @ref scripts/docs/en/userver/build/build.md
* @ref scripts/docs/en/userver/build/dependencies.md
* @ref scripts/docs/en/userver/build/options.md
* @ref scripts/docs/en/userver/build/userver.md


@anchor tutorial_services
## Tutorial
* @ref scripts/docs/en/userver/tutorial/hello_service.md
* @ref scripts/docs/en/userver/tutorial/config_service.md
* @ref scripts/docs/en/userver/tutorial/production_service.md
* @ref scripts/docs/en/userver/tutorial/tcp_service.md
* @ref scripts/docs/en/userver/tutorial/tcp_full.md
* @ref scripts/docs/en/userver/tutorial/http_caching.md
* @ref scripts/docs/en/userver/tutorial/flatbuf_service.md
* @ref scripts/docs/en/userver/tutorial/grpc_service.md
* @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md
* @ref scripts/docs/en/userver/tutorial/postgres_service.md
* @ref scripts/docs/en/userver/tutorial/mongo_service.md
* @ref scripts/docs/en/userver/tutorial/redis_service.md
* @ref scripts/docs/en/userver/tutorial/kafka_service.md
* @ref scripts/docs/en/userver/tutorial/auth_postgres.md
* @ref scripts/docs/en/userver/tutorial/digest_auth_postgres.md
* @ref scripts/docs/en/userver/tutorial/websocket_service.md
* @ref scripts/docs/en/userver/tutorial/multipart_service.md
* @ref scripts/docs/en/userver/tutorial/json_to_yaml.md


## Generic development
* @ref scripts/docs/en/userver/component_system.md
    * @ref userver_clients "Clients"
    * @ref userver_http_handlers "HTTP Handlers"
    * @ref userver_middlewares "HTTP Middlewares"
    * @ref userver_components "Other components"
* @ref scripts/docs/en/userver/synchronization.md
* @ref scripts/docs/en/userver/formats.md
* @ref scripts/docs/en/userver/chaotic.md
* @ref scripts/docs/en/userver/logging.md
* @ref scripts/docs/en/userver/task_processors_guide.md
* @ref scripts/docs/en/userver/periodics.md


## Testing and Benchmarking
* @ref scripts/docs/en/userver/testing.md
* @ref scripts/docs/en/userver/functional_testing.md
* @ref scripts/docs/en/userver/chaos_testing.md
* @ref scripts/docs/en/userver/profile_context_switches.md


## Protocols
* @ref scripts/docs/en/userver/grpc.md
* HTTP:
    * @ref clients::http::Client "Client"
    * @ref scripts/docs/en/userver/http_server.md
* @ref rabbitmq_driver
* Low level:
    * @ref engine::io::TlsWrapper "TLS client and server socket"
    * @ref engine::io::Socket "TCP and UDP sockets"
    * @ref engine::subprocess::ProcessStarter "Subprocesses"


## Runtime service features
* @ref scripts/docs/en/userver/dynamic_config.md
* @ref scripts/docs/en/schemas/dynamic_configs.md
* @ref scripts/docs/en/userver/log_level_running_service.md
* @ref scripts/docs/en/userver/requests_in_flight.md
* @ref scripts/docs/en/userver/service_monitor.md
* @ref scripts/docs/en/userver/memory_profile_running_service.md
* @ref scripts/docs/en/userver/dns_control.md
* @ref scripts/docs/en/userver/os_signals.md
* @ref scripts/docs/en/userver/deadline_propagation.md
* @ref scripts/docs/en/userver/congestion_control.md


## Caches
* @ref scripts/docs/en/userver/caches.md
* @ref scripts/docs/en/userver/cache_dumps.md
* @ref pg_cache
* @ref scripts/docs/en/userver/lru_cache.md


## PostgreSQL
* @ref pg_driver
* @ref pg_transactions
* @ref pg_run_queries
* @ref pg_process_results
* @ref scripts/docs/en/userver/pg_types.md
* @ref pg_user_row_types
* @ref pg_errors
* @ref pg_topology
* @ref scripts/docs/en/userver/pg_connlimit_mode_auto.md
* @ref scripts/docs/en/userver/pg_user_types.md


## MySQL
* @ref scripts/docs/en/userver/mysql/mysql_driver.md
* @ref scripts/docs/en/userver/mysql/supported_types.md
* @ref scripts/docs/en/userver/mysql/design_and_details.md


## Apache Kafka
* @ref scripts/docs/en/userver/kafka.md


## YDB
* @ref scripts/docs/en/userver/ydb.md


## Non relational databases
* @ref scripts/docs/en/userver/mongodb.md
* @ref scripts/docs/en/userver/redis.md
* @ref clickhouse_driver


## Opensource
* @ref scripts/docs/en/userver/development/stability.md
* @ref scripts/docs/en/userver/driver_guide.md
* @ref scripts/docs/en/userver/publications.md
* @ref CONTRIBUTING.md
* @ref SECURITY.md
* @ref scripts/docs/en/userver/security_changelog.md
* Distributed under [Apache-2.0 License](http://www.apache.org/licenses/LICENSE-2.0)
  * @ref THIRD_PARTY.md
