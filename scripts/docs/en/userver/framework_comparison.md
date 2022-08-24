## Feature Comparison with other Frameworks

If you find info in this table inaccurate, please [propose a PR with the fix][userver-docs-pr].

The table below shows features of different high-level asynchronous frameworks.
Note that the framework has to provide components well integrated
into each other. For example, a framework with "Async PotgreSQL", "Dynamic Config"
and "Metrics" has to have metrics for the PotgreSQL driver and dynamic configs
to control the driver behavior at runtime. Such framework gets the ✔️ in the
table below. If the components have weak integration with each other
or require additional work for such integration (the usual case for
third-party library), then the framework gets ± mark in the table below.
For missing functionality or if we found no info on the functionality we
use ❌ and ❓ respectively.


| Feature                           | 🐙 userver                                     | go-micro  4.7.0        | dapr 1.5.3                     | actix 0.13.0 + tokio 1.19.2 | drogon  1.7.5              |
|-----------------------------------|------------------------------------------------|-------------------------|-------------------------------|------------------------|----------------------------------|
| Programming model for IO-bound apps | stackful coroutines                            | stackful coroutines     | actors                        | stackless coroutines | callbacks / stackless coroutines |
| Programming language to use       | С++                                            | Go-lang                 | Python, JS, .Net, PHP, Java, Go | Rust                 | C++                              |
| Caching data from remote or DB    | ✔️ @ref md_en_userver_caches "[↗]"            | ❌                      | ❌                            | ❌                    | ❌                              |
| Dynamic Config @ref fcmp1 "[1]"   | ✔️ @ref md_en_schemas_dynamic_configs "[↗]"   | ✔️ [[↗]][gom-features]  | ❌                            | ❌                   | ❌                              |
| Unit testing                      | ✔️ C++ @ref md_en_userver_testing "[↗]"       | ✔️ via Go-lang          | ✔️ PHP [[↗]][dapr-testig]    | ✔️                    | ✔️ [[↗]][drog-testig]           |
| Functional Testing @ref fcmp2 "[2]" | ✔️ @ref md_en_userver_functional_testing "[↗]" | ❌     | ❌ [[↗]][dapr-testig]        | ❌ [[↗]][actix-test] | ❌ [[↗]][drog-testig]          |
| Async synchronization primitives  | ✔️ @ref md_en_userver_synchronization "[↗]"   | ✔️ via Go-lang          | ❌ [forces turn based access][dapr-actors]  | ✔️ [[↗]][tokio-sync] | ❌               |
| Dist locks                        | ✔️                                             | ✔️ [[↗]][gom-features] | ❌ [[↗]][dapr-distlock]       | ± third-party libs    | ❌                             |
| Async HTTP client                 | ✔️ @ref clients::http::Client "[↗]"           | ✔️                      | ✔️                            | ✔️                     | ✔️ [[↗]][drog-http-client]   |
| Async HTTP server                 | ✔️ @ref components::Server "[↗]"              | ✔️                      | ✔️                            | ✔️                     | ✔️                             |
| Async gRPC client                 | ✔️ @ref md_en_userver_grpc "[↗]"              | ✔️                      | ✔️                            | ± third-party libs     | ❌                            |
| Async gRPC server                 | ✔️ @ref md_en_userver_grpc "[↗]"              | ✔️                      | ✔️                            | ± third-party libs     | ❌                            |
| Async PotgreSQL                   | ✔️ @ref pg_driver "[↗]"                       | ± third-party driver   | ✔️ [[↗]][dapr-postgre]       | ❌ [manual offloading][acti-db] | ✔️ [[↗]][drog-db]    |
| PotgreSQL pipelining, binary protocol | ✔️ @ref pg_driver "[↗]"                   | ❌                      | ❌                            | ± third-party libs     | ❌                            |
| Async Redis                       | ✔️ @ref md_en_userver_redis "[↗]"             | ± third-party driver   | ✔️ [[↗]][dapr-redis]         | ± third-party libs      | ✔️ [[↗]][drog-redis]         |
| Async Mongo                       | ✔️ @ref md_en_userver_mongodb "[↗]"           | ± third-party driver   | ✔️ [[↗]][dapr-mongo]         | ❌ [manual offloading][acti-db] | ❌ [[↗]][drog-db]    |
| Async ClickHouse                  | ✔️ @ref clickhouse_driver "[↗]"               | ± third-party driver   | ❌                            | ± third-party libs      | ❌ [[↗]][drog-db]            |
| Async MySQL                       | ❌                                             | ± third-party driver   | ✔️ [[↗]][dapr-mysql]         | ❌ [[↗]][acti-db]      | ✔️ [[↗]][drog-db]            |
| Metrics                           | ✔️ @ref md_en_userver_service_monitor "[↗]"   | ± third-party driver   | ✔️ [[↗]][dapr-configs]       | ❌                      | ❌                            |
| No args evaluation for disabled logs | ✔️ @ref md_en_userver_logging "[↗]"        | ❌                      | ❌                            | ± third-party libs       | ❌                           |
| Secrets Management                | ± @ref storages::secdist::SecdistConfig "[↗]"  | ❓                      | ✔️                            | ❓                      | ❓                          |
| Distributed Tracing               | ✔️ @ref md_en_userver_logging "[↗]"           | ❓                      | ✔️ [[↗]][dapr-configs]       | ± third-party libs       | ❌                           |
| JSON, BSON, YAML                  | ✔️ @ref md_en_userver_formats "[↗]"           | ± third-party libs       | ± third-party libs            | ± third-party libs       | ± only JSON                  |
| Content compression/decompression | ✔️                                             | ✔️                      | ❓                            | ✔️                      | ✔️                          | 
| Service Discovery                 | ✔️ DNS, DB topology discovery                  | ✔️ [[↗]][gom-features]  | ❓                            | ❓                      | ❓                          |
| Async TCP/UDP                     | ✔️ @ref engine::io::Socket "[↗]"              | ✔️                      | ❓                            | ✔️ [[↗]][tokio-net]     | ❌                           |
| Async TLS Socket                  | ✔️ @ref engine::io::TlsWrapper "[↗]"          | ✔️                      | ❓                            | ± third-party libs       | ❌                           |
| Async HTTPS client                | ✔️ @ref clients::http::Client "[↗]"           | ✔️                      | ❓                            | ✔️                      | ❓                          |
| Async HTTPS server                | ❌                                             | ❓                      | ❓                            | ✔️                      | ❓                          |
| Deadlines and Cancellations       | ✔️                                             | ❓                      | ❓                            | ❓                      | ± [[↗]][drog-timeout]      |
| Retries and Load Balancing        | ✔️                                             | ✔️ [[↗]][gom-features] | ✔️                            | ❓                      |❓                          |


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
[actix-test]: https://actix.rs/docs/testing/
[acti-db]: https://actix.rs/docs/databases/
[drog-testig]: https://drogon.docsforge.com/master/testing-framework/
[drog-http-client]: https://drogon.docsforge.com/master/api/drogon/HttpClient/
[drog-db]: https://drogon.docsforge.com/master/database-general/
[drog-redis]: https://drogon.docsforge.com/master/redis/
[drog-timeout]: https://drogon.docsforge.com/master/session/
[tokio-sync]: https://docs.rs/tokio/0.2.18/tokio/sync/index.html
[tokio-net]: https://docs.rs/tokio/0.1.22/tokio/net/index.html

@anchor fcmp1 [1]: "Dynamic Configs" stands for any out-of-the-box functionality
that allows to change behavior of the service without downtime and restart.

@anchor fcmp2 [2]: Functional Testing includes DB startup and initialization; mocks for other
microservices; testpoints functionality.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_intro | @ref md_en_userver_supported_platforms ⇨
@htmlonly </div> @endhtmlonly
