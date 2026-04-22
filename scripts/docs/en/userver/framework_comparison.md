## Feature Comparison with other Frameworks

If you find info in this table inaccurate, please [propose a PR with the fix][userver-docs-pr].

The table below shows features of different high-level asynchronous frameworks.
Note that the framework has to provide components well integrated
into each other. For example, a framework with "Async PostgreSQL", "Dynamic Config"
and "Metrics" has to have metrics for the PostgreSQL driver and dynamic configs
to control the driver behavior at runtime. Such framework gets the ✔️ in the
table below. If the components have weak integration with each other
or require additional work for such integration (the usual case for
third-party library), then the framework gets ± mark in the table below.
For missing functionality or if we found no info on the functionality we
use ❌ and ❓ respectively.


| Feature                                | 🐙 userver                                                  | go-micro  4.7.0        | dapr 1.5.3                                | actix 0.13.0 + tokio 1.19.2    | drogon  1.7.5                    | POCO 1.13.3             |
|----------------------------------------|-------------------------------------------------------------|------------------------|-------------------------------------------|--------------------------------|----------------------------------|-------------------------|
| Programming model for IO-bound apps    | stackful coroutines                                         | stackful coroutines    | actors                                    | stackless coroutines           | callbacks / stackless coroutines | thread pool             |
| Programming language to use            | С++                                                         | Go-lang                | Python, JS, .Net, PHP, Java, Go           | Rust                           | C++                              | C++                     |
| Caching data from remote or DB         | ✔️ @ref scripts/docs/en/userver/caches.md "[↗]"             | ❌                      | ❌                                         | ❌                              | ❌                                | ✔️ [[↗]][poco-cache]    |
| Dynamic Config @ref fcmp1 "[1]"        | ✔️ @ref scripts/docs/en/dynamic_configs/dynamic_configs.md "[↗]" | ✔️ [[↗]][gom-features] | ❌                                         | ❌                              | ❌                                | ❌                       |
| Unit testing                           | ✔️ C++ @ref scripts/docs/en/userver/testing.md "[↗]"        | ✔️ via Go-lang         | ✔️ PHP [[↗]][dapr-testig]                 | ✔️                             | ✔️ [[↗]][drog-testig]            | ❓                       |
| Functional Testing @ref fcmp2 "[2]"    | ✔️ @ref scripts/docs/en/userver/functional_testing.md "[↗]" | ❌                      | ❌ [[↗]][dapr-testig]                      | ❌ [[↗]][actix-test]            | ❌ [[↗]][drog-testig]             | ❓                       |
| Async synchronization primitives       | ✔️ @ref scripts/docs/en/userver/synchronization.md "[↗]"    | ✔️ via Go-lang         | ❌ [forces turn based access][dapr-actors] | ✔️ [[↗]][tokio-sync]           | ❌                                | ✔️ [[↗]][poco-sync]     |
| Dist locks                             | ✔️                                                          | ✔️ [[↗]][gom-features] | ❌ [[↗]][dapr-distlock]                    | ± third-party libs             | ❌                                | ❓                       |
| Async HTTP client                      | ✔️ @ref clients::http::Client "[↗]"                         | ✔️                     | ✔️                                        | ✔️                             | ✔️ [[↗]][drog-http-client]       | ❓                       |
| Async HTTP server                      | ✔️ @ref components::Server "[↗]"                            | ✔️                     | ✔️                                        | ✔️                             | ✔️                               | ✔️ [[↗]][poco-net]      |
| Async gRPC client                      | ✔️ @ref scripts/docs/en/userver/grpc/grpc.md "[↗]"               | ✔️                     | ✔️                                        | ± third-party libs             | ❌                                | ❌                       |
| Async gRPC server                      | ✔️ @ref scripts/docs/en/userver/grpc/grpc.md "[↗]"               | ✔️                     | ✔️                                        | ± third-party libs             | ❌                                | ❌                       |
| Async PostgreSQL                       | ✔️ @ref scripts/docs/en/userver/pg_driver.md "[↗]"               | ± third-party driver   | ✔️ [[↗]][dapr-postgre]                    | ❌ [manual offloading][acti-db] | ✔️ [[↗]][drog-db]                | ✔️ [[↗]][poco-db]       |
| PostgreSQL pipelining, binary protocol | ✔️ @ref scripts/docs/en/userver/pg_driver.md "[↗]"               | ❌                      | ❌                                         | ± third-party libs             | ❌                                | ❓                       |
| Async Redis                            | ✔️ @ref scripts/docs/en/userver/redis.md "[↗]"              | ± third-party driver   | ✔️ [[↗]][dapr-redis]                      | ± third-party libs             | ✔️ [[↗]][drog-redis]             | ❓                       |
| Async Mongo                            | ✔️ @ref scripts/docs/en/userver/mongodb.md "[↗]"            | ± third-party driver   | ✔️ [[↗]][dapr-mongo]                      | ❌ [manual offloading][acti-db] | ❌ [[↗]][drog-db]                 | ❓                       |
| Async ClickHouse                       | ✔️ @ref clickhouse_driver "[↗]"                             | ± third-party driver   | ❌                                         | ± third-party libs             | ❌ [[↗]][drog-db]                 | ❓                       |
| Async MySQL                            | ✔️ @ref scripts/docs/en/userver/mysql/mysql_driver.md       | ± third-party driver   | ✔️ [[↗]][dapr-mysql]                      | ❌ [[↗]][acti-db]               | ✔️ [[↗]][drog-db]                | ✔️ [[↗]][poco-db]       |
| Async ODBC                             | ❌                                                           | ❓                      | ❓                                         | ❓                              | ❓                                | ✔️ [[↗]][poco-db]       |
| Metrics                                | ✔️ @ref scripts/docs/en/userver/service_monitor.md "[↗]"    | ± third-party driver   | ✔️ [[↗]][dapr-configs]                    | ❌                              | ❌                                | ❓                       |
| No args evaluation for disabled logs   | ✔️ @ref scripts/docs/en/userver/logging.md "[↗]"            | ❌                      | ❌                                         | ± third-party libs             | ❌                                | ❌                       |
| Secrets Management                     | ± @ref storages::secdist::SecdistConfig "[↗]"               | ❓                      | ✔️                                        | ❓                              | ❓                                | ❓                       |
| Distributed Tracing                    | ✔️ @ref scripts/docs/en/userver/logging.md "[↗]"            | ❓                      | ✔️ [[↗]][dapr-configs]                    | ± third-party libs             | ❌                                | ❓                       |
| JSON, BSON, YAML                       | ✔️ @ref scripts/docs/en/userver/formats.md "[↗]"            | ± third-party libs     | ± third-party libs                        | ± third-party libs             | ± only JSON                      | ❓                       |
| XML                                    | ❌                                                           | ❓                      | ❓                                         | ❓                              | ❓                                | ✔️ [[↗]][poco-xml]      |
| Content compression/decompression      | ✔️                                                          | ✔️                     | ❓                                         | ✔️                             | ✔️                               | ✔️ [[↗]][poco-streams]  |
| Service Discovery                      | ✔️ DNS, DB topology discovery                               | ✔️ [[↗]][gom-features] | ❓                                         | ❓                              | ❓                                | ± [[↗]][poco-net] (DNS) |
| Async TCP/UDP                          | ✔️ @ref engine::io::Socket "[↗]"                            | ✔️                     | ❓                                         | ✔️ [[↗]][tokio-net]            | ❌                                | ✔️ [[↗]][poco-net]      |
| Async TLS Socket                       | ✔️ @ref engine::io::TlsWrapper "[↗]"                        | ✔️                     | ❓                                         | ± third-party libs             | ❌                                | ✔️ [[↗]][poco-net]      |
| Async HTTPS client                     | ✔️ @ref clients::http::Client "[↗]"                         | ✔️                     | ❓                                         | ✔️                             | ❓                                | ✔️ [[↗]][poco-net]      |
| Async HTTPS server                     | ✔️ @ref components::Server "[↗]"                            | ❓                      | ❓                                         | ✔️                             | ❓                                | ✔️ [[↗]][poco-net]      |
| WebSockets Server                      | ✔️ @ref components::Server "[↗]"                            | ± third-party libs     | ❌ [[↗]][dapr-websock]                     | ± third-party libs             | ✔️ [[↗]][drogon]                 | ❌                       |
| Deadlines and Cancellations            | ✔️                                                          | ❓                      | ❓                                         | ❓                              | ± [[↗]][drog-timeout]            | ❌                       |
| Retries and Load Balancing             | ✔️                                                          | ✔️ [[↗]][gom-features] | ✔️                                        | ❓                              | ❓                                | ❌                       |


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
[dapr-websock]: https://github.com/dapr/dapr/issues/5766
[actix-test]: https://actix.rs/docs/testing/
[acti-db]: https://actix.rs/docs/databases/
[drogon]: https://github.com/drogonframework/drogon
[drog-testig]: https://drogon.docsforge.com/master/testing-framework/
[drog-http-client]: https://drogon.docsforge.com/master/api/drogon/HttpClient/
[drog-db]: https://drogon.docsforge.com/master/database-general/
[drog-redis]: https://drogon.docsforge.com/master/redis/
[drog-timeout]: https://drogon.docsforge.com/master/session/
[tokio-sync]: https://docs.rs/tokio/0.2.18/tokio/sync/index.html
[tokio-net]: https://docs.rs/tokio/0.1.22/tokio/net/index.html
[poco-cache]: https://pocoproject.org/slides/140-Cache.pdf
[poco-xml]: https://pocoproject.org/slides/170-XML.pdf
[poco-net]: https://pocoproject.org/slides/200-Network.pdf
[poco-sync]: https://pocoproject.org/slides/130-Threads.pdf
[poco-streams]: https://pocoproject.org/slides/100-Streams.pdf
[poco-db]: https://docs.pocoproject.org/current/Poco.Data.html

The table above shows the well-developed functionality of userver. Additionally userver is highly effective which is proven by the results of the Techempower benchmark run. Userver comes ahead of its competitors from the table. @ref bnchrs "[3]", see [[↗]][techempower-run]

![top15](/top15.png)

[techempower-run]: https://www.techempower.com/benchmarks/#section=test&runid=3c2e9871-9c2a-4ff3-bc31-620f65da4e74&hw=ph&test=composite

@anchor fcmp1 [1]: "Dynamic Configs" stands for any out-of-the-box functionality
that allows to change behavior of the service without downtime and restart.

@anchor fcmp2 [2]: Functional Testing includes DB startup and initialization; mocks for other
microservices; testpoints functionality.

@anchor bnchrs [3]: Techempower run № 3c2e9871-9c2a-4ff3-bc31-620f65da4e74.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/intro.md | @ref scripts/docs/en/userver/supported_platforms.md ⇨
@htmlonly </div> @endhtmlonly
