## The C++ Asynchronous Framework

🐙 **userver** is the modern open source asynchronous framework with a rich set of abstractions
for fast and comfortable creation of C++ microservices, services and utilities.

The framework solves the problem of efficient I/O interactions transparently for
the developers. As a result, with the framework you get straightforward source code,
avoid CPU-consuming context switches from OS, efficiently
utilize the CPU with a small amount of execution threads.

Telegram support chats: [English-speaking](https://t.me/userver_en) | 
[Russian-speaking](https://t.me/userver_ru) | [News channel](https://t.me/userver_news).


## Introduction
* @ref md_en_userver_intro_io_bound_coro
* @ref md_en_userver_intro
* @ref md_en_userver_framework_comparison
* @ref md_en_userver_supported_platforms
* @ref md_en_userver_tutorial_build
* @ref md_en_userver_beta_state
* @ref md_en_userver_roadmap_and_changelog


## Tutorial
* @ref md_en_userver_tutorial_hello_service
* @ref md_en_userver_tutorial_config_service
* @ref md_en_userver_tutorial_production_service
* @ref md_en_userver_tutorial_tcp_service
* @ref md_en_userver_tutorial_tcp_full
* @ref md_en_userver_tutorial_http_caching
* @ref md_en_userver_tutorial_flatbuf_service
* @ref md_en_userver_tutorial_grpc_service
* @ref md_en_userver_tutorial_postgres_service
* @ref md_en_userver_tutorial_mongo_service
* @ref md_en_userver_tutorial_redis_service
* @ref md_en_userver_tutorial_auth_postgres


## Generic development
* @ref md_en_userver_component_system
    * @ref userver_clients "Clients"
    * @ref userver_http_handlers "HTTP Handlers"
    * @ref userver_components "Other components"
* @ref md_en_userver_synchronization
* @ref md_en_userver_formats
* @ref md_en_userver_logging
* @ref md_en_userver_task_processors_guide


## Testing and Benchmarking
* @ref md_en_userver_testing
* @ref md_en_userver_functional_testing
* @ref md_en_userver_chaos_testing
* @ref md_en_userver_profile_context_switches


## Protocols
* @ref md_en_userver_grpc
* HTTP:
    * @ref clients::http::Client "Client"
    * @ref components::Server "Server"
* @ref rabbitmq_driver
* Low level:
    * @ref engine::io::TlsWrapper "TLS client and server socket"
    * @ref engine::io::Socket "TCP and UDP sockets"
    * @ref engine::subprocess::ProcessStarter "Subprocesses"


## Runtime service features
* @ref md_en_schemas_dynamic_configs
* @ref md_en_userver_log_level_running_service
* @ref md_en_userver_requests_in_flight
* @ref md_en_userver_service_monitor
* @ref md_en_userver_memory_profile_running_service
* @ref md_en_userver_dns_control
* @ref md_en_userver_os_signals


## Caches
* @ref md_en_userver_caches
* @ref md_en_userver_cache_dumps
* @ref pg_cache
* @ref md_en_userver_lru_cache


## PostgreSQL
* @ref pg_driver
* @ref pg_transactions
* @ref pg_run_queries
* @ref pg_process_results
* @ref pg_types
* @ref pg_user_row_types
* @ref pg_errors
* @ref pg_topology
* @ref pg_user_types
  * @ref pg_composite_types
  * @ref pg_enum
  * @ref pg_range_types
  * @ref pg_arrays
  * @ref pg_bytea

## MySQL
* @ref mysql_driver
* @ref md_en_userver_mysql_supported_types
* @ref md_en_userver_mysql_design_and_details


## Non relational databases
* @ref md_en_userver_mongodb
* @ref md_en_userver_redis
* @ref clickhouse_driver


## Opensource
* @ref md_en_userver_development_stability
* @ref md_en_userver_development_releases
* @ref md_en_userver_driver_guide
* @ref md_en_userver_publications
* @ref CONTRIBUTING.md
* @ref SECURITY.md
* @ref md_en_userver_security_changelog
* Distributed under [Apache-2.0 License](http://www.apache.org/licenses/LICENSE-2.0)
  * @ref THIRD_PARTY.md
