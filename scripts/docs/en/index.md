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

Samples and the source codes of the framework itself are available at the
[userver-framework at github](https://github.com/userver-framework/).

Mirror is available at [the SourceCraft](https://sourcecraft.dev/userver/repos).


## Introduction

* @ref scripts/docs/en/userver/intro_io_bound_coro.md
* @ref scripts/docs/en/userver/intro.md
* @ref scripts/docs/en/userver/framework_comparison.md
* @ref scripts/docs/en/userver/supported_platforms.md
* @ref scripts/docs/en/userver/deploy_env.md
* @ref scripts/docs/en/userver/development/releases.md
* @ref scripts/docs/en/userver/roadmap_and_changelog.md
* @ref scripts/docs/en/userver/distro_maintainers.md
* @ref scripts/docs/en/userver/faq.md


@anchor Install
## Install

* @ref scripts/docs/en/userver/build/build.md
* @ref scripts/docs/en/userver/build/dependencies.md
* @ref scripts/docs/en/userver/build/options.md
* @ref scripts/docs/en/userver/build/userver.md


@anchor tutorial_services
## Tutorial

@note Before tackling domain-specific problems,
@ref scripts/docs/en/userver/build/build.md "create a service project"
and make sure that it builds and passes tests.

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
* @ref scripts/docs/en/userver/tutorial/scylla_service.md
* @ref scripts/docs/en/userver/tutorial/redis_service.md
* @ref scripts/docs/en/userver/tutorial/kafka_service.md
* @ref scripts/docs/en/userver/tutorial/auth_postgres.md
* @ref scripts/docs/en/userver/tutorial/digest_auth_postgres.md
* @ref scripts/docs/en/userver/tutorial/websocket_service.md
* @ref scripts/docs/en/userver/tutorial/websocket_client.md
* @ref scripts/docs/en/userver/tutorial/static_content.md
* @ref scripts/docs/en/userver/tutorial/multipart_service.md
* @ref scripts/docs/en/userver/tutorial/s3api.md
* @ref scripts/docs/en/userver/tutorial/json_to_yaml.md


## Generic development
* @ref scripts/docs/en/userver/component_system.md
    * @ref userver_clients "Clients"
    * @ref userver_http_handlers "HTTP Handlers"
    * @ref userver_middlewares "HTTP Middlewares"
    * @ref userver_components "Other components"
* @ref scripts/docs/en/userver/synchronization.md
* @ref scripts/docs/en/userver/formats.md
* @ref scripts/docs/en/userver/logging.md
* @ref scripts/docs/en/userver/task_processors_guide.md
* @ref scripts/docs/en/userver/periodics.md

## Code generation
* @ref scripts/docs/en/userver/codegen_overview.md
* @ref scripts/docs/en/userver/chaotic.md
* @ref scripts/docs/en/userver/sql_files.md


## Testing and Benchmarking
* @ref scripts/docs/en/userver/testing.md
* @ref scripts/docs/en/userver/functional_testing.md
* @ref scripts/docs/en/userver/chaos_testing.md
* @ref scripts/docs/en/userver/profile_context_switches.md
* @ref scripts/docs/en/userver/gdb_debugging.md


## Protocols
* @ref scripts/docs/en/userver/grpc/grpc.md
    * @ref scripts/docs/en/userver/grpc/timeouts_retries.md
    * @ref scripts/docs/en/userver/grpc/rich_status.md
    * Middlewares
        * @ref scripts/docs/en/userver/grpc/server_middlewares.md
            * @ref scripts/docs/en/userver/grpc/server_middleware_implementation.md
        * @ref scripts/docs/en/userver/grpc/client_middlewares.md
            * @ref scripts/docs/en/userver/grpc/client_middleware_implementation.md
        * @ref scripts/docs/en/userver/grpc/middlewares_order.md
        * @ref scripts/docs/en/userver/grpc/middlewares_toggle.md
        * @ref scripts/docs/en/userver/grpc/middlewares_configuration.md
* HTTP:
    * @ref clients::http::Client "Client"
    * @ref scripts/docs/en/userver/http_server.md
* @ref scripts/docs/en/userver/rabbitmq_driver.md
* Low level:
    * @ref engine::io::TlsWrapper "TLS client and server socket"
    * @ref engine::io::Socket "TCP and UDP sockets"
    * @ref engine::subprocess::ProcessStarter "Subprocesses"


## Runtime service features
* @ref scripts/docs/en/userver/dynamic_config.md
    * @ref scripts/docs/en/dynamic_configs/dynamic_configs.md
* @ref scripts/docs/en/userver/log_level_running_service.md
* @ref scripts/docs/en/userver/requests_in_flight.md
* @ref scripts/docs/en/userver/metrics.md
* @ref scripts/docs/en/userver/service_monitor.md
* @ref scripts/docs/en/userver/memory_profile_running_service.md
* @ref scripts/docs/en/userver/dns_control.md
* @ref scripts/docs/en/userver/os_signals.md
* @ref scripts/docs/en/userver/deadline_propagation.md
* @ref scripts/docs/en/userver/congestion_control.md
* @ref scripts/docs/en/userver/stack.md
* @ref scripts/docs/en/userver/dump_coroutines.md
* @ref scripts/docs/en/userver/long_transactions.md
* @ref scripts/docs/en/userver/graceful_shutdown.md


## Caches
* @ref scripts/docs/en/userver/caches.md
* @ref scripts/docs/en/userver/cache_dumps.md
* @ref scripts/docs/en/userver/pg/cache.md
* @ref scripts/docs/en/userver/lru_cache.md


## PostgreSQL
* @ref scripts/docs/en/userver/pg/driver.md
* @ref scripts/docs/en/userver/pg/transactions.md
* @ref scripts/docs/en/userver/pg/run_queries.md
* @ref scripts/docs/en/userver/pg/process_results.md
* @ref scripts/docs/en/userver/pg/types.md
* @ref scripts/docs/en/userver/pg/user_row_types.md
* @ref scripts/docs/en/userver/pg/errors.md
* @ref scripts/docs/en/userver/pg/topology.md
* @ref scripts/docs/en/userver/pg/connlimit_mode_auto.md
* @ref scripts/docs/en/userver/pg/user_types.md


## MySQL
* @ref scripts/docs/en/userver/mysql/mysql_driver.md
* @ref scripts/docs/en/userver/mysql/supported_types.md
* @ref scripts/docs/en/userver/mysql/design_and_details.md


## Apache Kafka
* @ref scripts/docs/en/userver/kafka.md


## YDB
* @ref scripts/docs/en/userver/ydb.md


## SQLite
* @ref scripts/docs/en/userver/sqlite/sqlite_driver.md
* @ref scripts/docs/en/userver/sqlite/supported_types.md
* @ref scripts/docs/en/userver/sqlite/design_and_details.md


## ODBC
* @ref scripts/docs/en/userver/odbc.md


## Non relational databases
* @ref scripts/docs/en/userver/mongodb.md
* @ref scripts/docs/en/userver/scylladb.md
* @ref scripts/docs/en/userver/redis.md
* @ref scripts/docs/en/userver/clickhouse/driver.md

## Libraries
* @ref scripts/docs/en/userver/libraries/easy.md
* @ref scripts/docs/en/userver/libraries/s3api.md
* @ref scripts/docs/en/userver/libraries/grpc-reflection.md
* @ref scripts/docs/en/userver/libraries/multi_index_lru.md

## Opensource
* @ref scripts/docs/en/userver/development/stability.md
* @ref scripts/docs/en/userver/driver_guide.md
* @ref scripts/docs/en/userver/publications.md
* @ref CONTRIBUTING.md
* @ref SECURITY.md
* @ref scripts/docs/en/userver/security_changelog.md
* Distributed under [Apache-2.0 License](http://www.apache.org/licenses/LICENSE-2.0)
  * @ref THIRD_PARTY.md
