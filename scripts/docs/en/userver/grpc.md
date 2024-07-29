# gRPC

**Quality:** @ref QUALITY_TIERS "Platinum Tier".

## Introduction

🐙 **userver** provides a gRPC driver as `userver-grpc` library. It uses ```namespace ugrpc::client``` and ```namespace ugrpc::server```.

The driver wraps `grpcpp` in the userver asynchronous interface.

See also:
* @ref scripts/docs/en/userver/tutorial/grpc_service.md
* [Official gRPC documentation](https://grpc.io/docs/languages/cpp/)
* [grpcpp reference](https://grpc.github.io/grpc/cpp/index.html)
* [grpc-core reference](https://grpc.github.io/grpc/core/index.html)

## Capabilities

* Creating asynchronous gRPC clients and services;
* Forwarding gRPC Core logs to userver logs;
* Caching and reusing connections;
* Timeouts;
* Collection of metrics on driver usage;
* Cancellation support;
* Automatic authentication using middlewares;
* @ref scripts/docs/en/userver/deadline_propagation.md .

## Installation

Generate and link a library from `.proto` schemas and link to it in your `CMakeLists.txt`:

@snippet samples/grpc_service/CMakeLists.txt  gRPC sample - CMake

`userver_add_grpc_library` will link `userver-grpc` transitively and will generate the usual `.pb.h + .pb.cc` files. For service definitions, it will additionally generate asynchronous interfaces `foo_client.usrv.pb.hpp` and `foo_service.usrv.pb.hpp`.

To create gRPC clients in your microservice, register the provided ugrpc::client::ClientFactoryComponent and add the corresponding component section to the static config.

To create gRPC services in your microservice, register the provided ugrpc::server::ServerComponent and add the corresponding component section to the static config.

## gRPC clients

### Client creation

In a component constructor, find ugrpc::client::ClientFactoryComponent and store a reference to its ugrpc::client::ClientFactory. Using it, you can create gRPC clients of code-generated `YourServiceClient` types.

Client creation in an expensive operation! Either create them once at the server boot time or cache them.

### Client usage

Typical steps include:
* Filling a `std::unique_ptr<grpc::ClientContext>` with request settings
    * gRPC documentation recommends using `set_deadline` for each RPC
    * Fill the authentication metadata as necessary
* Stream creation by calling a client method
* Operations on the stream
* Depending on the RPC kind, it is necessary to call `Finish` or `Read` until it returns `false` (otherwise the connection will close abruptly)

Read the documentation on gRPC streams:
* Single request, single response ugrpc::client::UnaryCall
* Single request, response stream ugrpc::client::InputStream
* Request stream, single response ugrpc::client::OutputStream
* Request stream, response stream ugrpc::client::BidirectionalStream

On errors, exceptions from userver/ugrpc/client/exceptions.hpp are thrown. It is recommended to catch them outside the entire stream interaction. You can catch exceptions for [specific gRPC error codes](https://grpc.github.io/grpc/core/md_doc_statuscodes.html) or all at once.

### TLS / SSL

May be enabled via

```
# yaml
components_manager:
    components:
        grpc-client-factory:
            auth-type: ssl
```

Available values are:

- `insecure` (default)
- `ssl`

SSL has to be disabled in tests, because it
requires the server to have a public domain name, which it does not in tests.
In testsuite, SSL in gRPC clients is disabled automatically.

## gRPC services

### Service creation

A service implementation is a class derived from a code-generated `YourServiceBase` interface class. Each service method from the schema corresponds to a method of the interface class. If you don't override some of the methods, `UNIMPLEMENTED` error code will be reported for those.

To register your service:
* Create a component that derives from ugrpc::server::ServiceComponentBase
* Store your service implementation instance inside the component
* Call ugrpc::server::ServiceComponentBase::RegisterService in the component's constructor
* Don't forget to register your service component.

### Service method handling

Each method receives:
* A stream controller object, used to respond to the RPC
    * Also contains grpc::ClientContext from grpcpp library
* A request (for single-request RPCs only)

When using a server stream, always call `Finish` or `FinishWithError`. Otherwise the client will receive `UNKNOWN` error, which signifies an internal server error.

Read the documentation on gRPC streams:
* Single request, single response ugrpc::server::UnaryCall
* Request stream, single response ugrpc::server::InputStream
* Single request, response stream ugrpc::server::OutputStream
* Request stream, response stream ugrpc::server::BidirectionalStream

On connection errors, exceptions from userver/ugrpc/server/exceptions.hpp are thrown. It is recommended not to catch them, leading to RPC interruption. You can catch exceptions for [specific gRPC error codes](https://grpc.github.io/grpc/core/md_doc_statuscodes.html) or all at once.

### Custom server credentials

By default, gRPC server uses `grpc::InsecureServerCredentials`. To pass a custom credentials:

1. Do not pass `grpc-server.port` in the static config
2. Create a custom component, e.g. `GrpcServerConfigurator`
3. `context.FindComponent<ugrpc::server::ServerComponent>().GetServer()`
4. Call @ref ugrpc::server::Server::WithServerBuilder "WithServerBuilder"
   method on the returned @ref ugrpc::server::Server "server"
5. Inside the callback, call `grpc::ServerBuilder::AddListeningPort`,
   passing it your custom credentials
    * Look into grpc++ documentation and into
      `<grpcpp/security/server_credentials.h>` for available credentials
    * SSL credentials are `grpc::SslServerCredentials`

### Middlewares

The gRPC server can be extended by middlewares.
Middleware is called on each incoming (for service) or outgoing (for client) RPC request.
Different middlewares handle the call in the defined order.
A middleware may decide to reject the call or call the next middleware in the stack.
Middlewares may implement almost any enhancement to the gRPC server including authorization
and authentication, ratelimiting, logging, tracing, audit, etc.

Middlewares to use are indicated in static config in section `middlewares` of `ugrpc::server::ServiceComponentBase` descendant component.
Default middleware list for handlers can be specified in `grpc-server.service-defaults.middlewares` config section.

Example configuration:
```
components_manager:
    components:
        some-service-client:
            middlewares:
              - grpc-client-logging
              - grpc-client-deadline-propagation
              - grpc-client-baggage

        grpc-server:
            service-defaults:
                middlewares:
                  - grpc-server-logging
                  - grpc-server-deadline-propagation
                  - grpc-server-congestion-control
                  - grpc-server-baggage

        some-service:
            middlewares:
              # Completely overwrite the default list
              - grpc-server-logging

```

Use ugrpc::server::MiddlewareBase and ugrpc::client::MiddlewareBase to implement
new middlewares.


@anchor grpc_generic_api
## Generic API

gRPC generic API allows to call and accept RPCs with dynamic service and method names.
The other side will see this as a normal RPC, it does not need to use generic API.

Intended mainly for use in proxies. Metadata can be used to proxy the request without parsing it.

See details in:

* @ref ugrpc::client::GenericClient ;
* @ref ugrpc::server::GenericServiceBase .

Full example showing the usage of both:

* @ref samples/grpc-generic-proxy/src/proxy_service.hpp
* @ref samples/grpc-generic-proxy/src/proxy_service.cpp
* @ref samples/grpc-generic-proxy/main.cpp
* @ref samples/grpc-generic-proxy/static_config.yaml
* @ref samples/grpc-generic-proxy/config_vars.yaml
* @ref samples/grpc-generic-proxy/CMakeLists.txt

Based on:

* grpcpp [generic stub](https://grpc.github.io/grpc/cpp/grpcpp_2generic_2generic__stub_8h.html);
* grpcpp [generic service](https://grpc.github.io/grpc/cpp/grpcpp_2generic_2async__generic__service_8h.html).


## Metrics

* Client metrics are put inside `grpc.client.by-destination {grpc_destination=FULL_SERVICE_NAME/METHOD_NAME}`
* Server metrics are put inside `grpc.server.by-destination {grpc_destination=FULL_SERVICE_NAME/METHOD_NAME}`

These are the metrics provided for each gRPC method:

* `timings.1min` — time from RPC start to finish (`utils::statistics::Percentile`)
* `status` with label `grpc_code=STATUS_CODE_NAME` — RPCs that finished
  with specified status codes, one metric per gRPC status
* Metrics for RPCs that finished abruptly without a status:
   * `cancelled` — RPCs that were interrupted due to task cancellation.
     (Not to be confused with RPCs finished with `CANCELLED` status.)
     Server-side, this means that the client dropped the RPC or called
     `TryCancel`. Client-side, this likely means that either the parent
     handler was interrupted, or the RPC was dropped as unnecessary.
     See ugrpc::client::RpcCancelledError and
     ugrpc::server::RpcInterruptedError
   * `cancelled-by-deadline-propagation` — RPCs, the handling of which was
     interrupted because the deadline specified in the request was reached.
     (Available for both server and client-side.)
     See also @ref scripts/docs/en/userver/deadline_propagation.md "userver deadline propagation"
   * `network-error` — other RPCs that finished abruptly without a status,
     see ugrpc::client::RpcInterruptedError and
     ugrpc::server::RpcInterruptedError
* `abandoned-error` — RPCs that we forgot to `Finish`
  (always a bug in `ugrpc` usage). Such RPCs also separately report
  the status or network error that occurred during the automatic
  request termination
* `deadline-propagated` — RPCs, for which deadline was specified.
  See also @ref scripts/docs/en/userver/deadline_propagation.md "userver deadline propagation"
* `rps` — requests per second:
  ```
  sum(status) + network-error + cancelled + cancelled-by-deadline-propagation
  ```
* `eps` — server errors per second
  ```
  sum(status if is_error(status))
  ```
  The status codes to be considered server errors are chosen according to
  [OpenTelemetry recommendations](https://opentelemetry.io/docs/specs/semconv/rpc/grpc/#grpc-status)
   * `UNKNOWN`
   * `DATA_LOSS`
   * `UNIMPLEMENTED`
   * `INTERNAL`
   * `UNAVAILABLE`
   * Note: `network-error` is not accounted in `eps`, because either the client
     is responsible for the server dropping the request (`TryCancel`, deadline),
     or it is truly a network error, in which case it's typically helpful
     for troubleshooting to say that there are issues not with the uservice
     process itself, but with the infrastructure
* `active` — The number of currently active RPCs (created and not finished)


## Unit tests and benchmarks

* @ref scripts/docs/en/userver/tutorial/grpc_service.md shows how to test
  userver gRPC services and clients in gtest
* @ref ugrpc::tests::Service and ugrpc::tests::ServiceBase can be used
  to benchmark userver gRPC services and clients, as well as create more
  complex gtest tests with multiple services and perhaps databases.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/profile_context_switches.md | @ref rabbitmq_driver ⇨
@htmlonly </div> @endhtmlonly
