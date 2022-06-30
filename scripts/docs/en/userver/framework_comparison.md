## Feature Comparison with other Frameworks

If you find info in this table inaccurate, please [propose a PR with the fix][userver-docs-pr].

The table below shows features of different high-level asynchronous frameworks.

| Feature                           | 🐙 userver                                     | go-micro  4.7.0        | dapr 1.5.3                     | actix 0.13.0          |
|-----------------------------------|------------------------------------------------|-------------------------|-------------------------------|------------------------|
| Programming model for IO-bound apps | stackful coroutines                            | stackful coroutines     | actors                        | stackless coroutines    |
| Programming language to use       | С++                                            | Go-lang                 | Python, JS, .Net, PHP, Java, Go | Rust                   |
| Caching                           | ✔️ @ref md_en_userver_caches "[↗]"            | ❌                      | ❌                            | ❌                      |
| Dynamic Config                    | ✔️ @ref md_en_schemas_dynamic_configs "[↗]"   | ✔️ [[↗]][gom-features]  | ❌                            | ❌                      |
| Unit testing                      | ✔️ C++ @ref md_en_userver_testing "[↗]"       | ✔️ via Go-lang          | ✔️ PHP [[↗]][dapr-testig]    | ✔️                      |
| Functional testing                | ✔️ C++, Python  @ref md_en_userver_functional_testing "[↗]" | ±, no DB startup      | ± PHP, [no DB startup][dapr-testig] | ±, [no DB startup][actix-testing] |
| Async synchronization primitives  | ✔️ @ref md_en_userver_synchronization "[↗]"   | ✔️ via Go-lang          | ❌ [forces turn based access][dapr-actors]  | ❌                     |
| Dist locks                        | ✔️                                             | ✔️ [[↗]][gom-features] | ❌ [[↗]][dapr-distlock]       | ± third-party libs      |
| Async HTTP client                 | ✔️ @ref clients::http::Client "[↗]"           | ✔️                      | ✔️                            | ✔️                      |
| Async HTTP server                 | ✔️ @ref components::Server "[↗]"              | ✔️                      | ✔️                            | ✔️                      |
| Async gRPC client                 | ✔️ @ref md_en_userver_grpc "[↗]"              | ✔️                      | ✔️                            | ❌                      |
| Async gRPC server                 | ✔️ @ref md_en_userver_grpc "[↗]"              | ✔️                      | ✔️                            | ❌                      |
| Async PotgreSQL                   | ✔️ @ref pg_driver "[↗]"                       | ✔️ third-party driver   | ✔️ [[↗]][dapr-postgre]       | ❌ [manual offloading][acti-db] |
| PotgreSQL pipelining, binary protocol | ✔️ @ref pg_driver "[↗]"                       | ❌                      | ❌                            | ❌                       |
| Async Redis                       | ✔️ @ref md_en_userver_redis "[↗]"             | ✔️ third-party driver   | ✔️ [[↗]][dapr-redis]         | ❌ [[↗]][acti-db]               |
| Async Mongo                       | ✔️ @ref md_en_userver_mongodb "[↗]"           | ✔️ third-party driver   | ✔️ [[↗]][dapr-mongo]         | ❌ [manual offloading][acti-db] |
| Async ClickHouse                  | ✔️ @ref clickhouse_driver "[↗]"               | ✔️ third-party driver   | ❌                            | ❌ [[↗]][acti-db]               |
| Async MySQL                       | ❌                                             | ✔️ third-party driver   | ✔️ [[↗]][dapr-mysql]         | ❌ [[↗]][acti-db]       |
| Metrics                           | ✔️ @ref md_en_userver_service_monitor "[↗]"   | ✔️ third-party driver   | ✔️ [[↗]][dapr-configs]       | ❌                      |
| No args evaluation for disabled logs | ✔️ @ref md_en_userver_logging "[↗]"      | ❌                      | ❌                            | ± third-party libs       |
| Secrets Management                | ± @ref storages::secdist::SecdistConfig "[↗]"  | ❓                      | ✔️                            | ❓                      |
| Distributed tracing               | ✔️ @ref md_en_userver_logging "[↗]"           | ❓                      | ✔️ [[↗]][dapr-configs]       | ± third-party libs       |
| JSON, BSON, YAML                  | ✔️ @ref md_en_userver_formats "[↗]"           | ± third-party libs       | ± third-party libs            | ± third-party libs       |
| Content compression/decompression | ✔️                                             | ✔️                      | ❓                            | ✔️                      |
| Service Discovery                 | ✔️ DNS, DB topology discovery                  | ✔️ [[↗]][gom-features]  | ❓                            | ❓                      |
| Async TCP/UDP                     | ✔️ @ref engine::io::Socket "[↗]"              | ✔️                      | ❓                            | ❌                      |
| Async TLS Socket                  | ✔️ @ref engine::io::TlsWrapper "[↗]"          | ✔️                      | ❓                            | ❌                      |
| Async HTTPS client                | ✔️ @ref clients::http::Client "[↗]"           | ✔️                      | ❓                            | ✔️                      |
| Async HTTPS server                | ❌                                             | ❓                      | ❓                            | ✔️                      |
| Deadlines and Cancellations       | ✔️                                             | ❓                      | ❓                            | ❓                      |
| Retries and Load Balancing        | ✔️                                             | ✔️ [[↗]][gom-features]  | ✔️                            | ❓                      |


[userver-docs-pr]: https://github.com/userver-framework/userver/blob/develop/scripts/docs/en/userver/
[gom-features]: https://github.com/asim/go-micro#features
[dapr-configs]: https://docs.dapr.io/operations/configuration/configuration-overview/
[dapr-testig]: https://docs.dapr.io/developing-applications/sdks/php/php-app/php-unit-testing/
[dapr-actors]: https://docs.dapr.io/developing-applications/building-blocks/actors/actors-overview/
[dapr-mongo]: https://docs.dapr.io/reference/components-reference/supported-state-stores/setup-mongodb/
[dapr-redis]: https://docs.dapr.io/reference/components-reference/supported-state-stores/setup-redis/
[dapr-postgre]: https://docs.dapr.io/reference/components-reference/supported-state-stores/setup-postgresql/
[dapr-mysql]: https://docs.dapr.io/reference/components-reference/supported-state-stores/setup-mysql/
[dapr-distlock]: https://github.com/dapr/dapr/issues/3549
[actix-testing]: https://actix.rs/docs/testing/
[acti-db]: https://actix.rs/docs/databases/
