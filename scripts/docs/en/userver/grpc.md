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

@snippet samples/grpc_service/CMakeLists.txt  add_grpc_library

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

### Client middlewares

Client behaviour can be modified with a middleware. Middleware code is executed before or after the client code. 
Middlewares to use are indicated in static config in the defining component. For example:

```
# yaml
components_manager:
    components:
        grpc-client-factory:
            middlewares:
              - grpc-client-logging
              - grpc-client-deadline-propagation 
```

#### List of standard client middlewares

 1. `grpc-client-logging` with component ugrpc::client::middlewares::log::Component - logs requests and responses.
 2. `grpc-client-deadline-propagation` with component ugrpc::client::middlewares::deadline_propagation::Component - activates 
 @ref scripts/docs/en/userver/deadline_propagation.md.
 3. `grpc-client-baggage` with component ugrpc::client::middlewares::baggage::Component - passes request baggage to subrequests.
 
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

### Server middlewares

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

#### List of standard server middlewares:

  1. `grpc-server-logging` with component ugrpc::server::middlewares::log::Component - logs requests.
  2. `grpc-server-deadline-propagation` with component ugrpc::server::middlewares::deadline_propagation::Component - activates 
  @ref scripts/docs/en/userver/deadline_propagation.md.
  3. `grpc-server-congestion-control` with component ugrpc::server::middlewares::congestion_control::Component - limits requests.
  See Congestion Control section of @ref scripts/docs/en/userver/tutorial/production_service.md.
  4. `grpc-server-baggage` with component ugrpc::server::middlewares::baggage::Component - passes request baggage to subrequests.
  5. `grpc-server-headers-propagator` with component ugrpc::server::middlewares::headers_propagator::Component - propagates headers.

## gRPC Logs

Each gRPC call generates a span ( `tracing::Span` ) containing tags which are inherited by all child logs. 

Additionally, if logging is activated, a separate log is generated for every gRPC request-response in `grpc-server-logging` and `grpc-client-logging` middlewares.

gRPC logs are listed below.

### Client log tags

Middleware component name          | Key                    | Value
---------------------------------- |----------------------- | ------------------------
builtin                            | `error`                | error flag, `true`
builtin                            | `grpc_code`            | error code  `grpc::StatusCode`, [list](https://grpc.github.io/grpc/core/md_doc_statuscodes.html) 
builtin                            | `error_msg`            | error message
`grpc-client-logging`              | `meta_type`            | call name
`grpc-client-logging`              | `grpc_type`            | `request` or `response`
`grpc-client-logging`              | `body`                 | logged message body
`grpc-client-deadline-propagation` | `deadline_updated`     | error flag, `true`
`grpc-client-deadline-propagation` | `timeout_ms`           | amount of time before request deadline

### Server log tags

Middleware component name          | Key         | Value
---------------------------------- |------------ | --------------------------
builtin                            | `error`     | error flag `true`
builtin                            | `error_msg` | error message
`grpc-server-logging`              | `meta_type` | call name
`grpc-server-logging`              | `body`      | logged message body
`grpc-server-logging`              | `type`      | `request` or `response`
`grpc-server-logging`              | `grpc_type` | `request` or `response`
`grpc-server-deadline-propagation` | `received_deadline_ms` | deadline
`grpc-server-deadline-propagation` | `cancelled_by_deadline` | `true` or `false`
`grpc-server-congestion-control`   | `limit` | rate limit when request is throttled (tokens per second)
`grpc-server-congestion-control`   | `service/method` | method name when request is throttled
`grpc-server-baggage`              | `Got baggage header: ` | baggage header

### Hiding fields in request-response logs

The gRPC driver provides log fields hiding in request-response logs. You need to do the following:

1) Add [userver/field_options.proto](https://github.com/userver-framework/userver/blob/develop/grpc/proto/userver/field_options.proto) to your service. Generate a library from this proto file and link against it in your `CMakeLists.txt`.

2) Import userver/field_options.proto in your proto file.

3) Use `[(userver.field).secret = true]` opposite to the filds that you want to hide. In the following example the fields `password` and `secret_code` will be hidden in the logs:

```proto
message Creds {
  string login = 1;
  string password = 2 [(userver.field).secret = true];
  string secret_code = 3 [(userver.field).secret = true];
}
```

### grpc-core logs

grpc-core is a lower level library, its logs are forwarded to the userver default logger. In this process only error level logs get through from grpc-core to the userver default logger if the default settings are used. However, the default settings can be overriden and more verbose logging can be achieved. 

To do this you need to change the value of `native-log-level` in the static config file in the components `grpc-client-common` and `grpc-server`:

```
grpc-server:
    native-log-level: debug
```

There are 3 possible logging levels: `error`, `info`, `debug`.
If logging level is set in several components then the most verbose logging level is used.


@anchor grpc_generic_api
## Generic API

gRPC generic API allows to call and accept RPCs with dynamic service and method names.
The other side will see this as a normal RPC, it does not need to use generic API.

Intended mainly for use in proxies. Metadata can be used to proxy the request without parsing it.

See details in:

* @ref ugrpc::client::GenericClient ;
* @ref ugrpc::server::GenericServiceBase .

Full example showing the usage of both:

* @ref grpc-generic-proxy/src/proxy_service.hpp
* @ref grpc-generic-proxy/src/proxy_service.cpp
* @ref grpc-generic-proxy/main.cpp
* @ref grpc-generic-proxy/static_config.yaml
* @ref grpc-generic-proxy/config_vars.yaml
* @ref grpc-generic-proxy/CMakeLists.txt

Based on:

* grpcpp [generic stub](https://grpc.github.io/grpc/cpp/grpcpp_2generic_2generic__stub_8h.html);
* grpcpp [generic service](https://grpc.github.io/grpc/cpp/grpcpp_2generic_2async__generic__service_8h.html).


## Metrics

* Client metrics are put inside `grpc.client.by-destination`
* Server metrics are put inside `grpc.server.by-destination`
* Client-wide totals are currently NOT computed
* Server-wide totals are put inside `grpc.server.total`

Each metric has the following labels:

* `grpc_service` - fully qualified grpc (proto) service name
* `grpc_method` - fully qualified grpc method name
* `grpc_destination` = `grpc_service/grpc_method`
* `grpc_destination_full` = `client_name/grpc_service/grpc_method` (only for client metrics)

These are the metrics provided for each gRPC method:

* `timings.1min` — time from RPC start to finish (`utils::statistics::Percentile`)
* `status` with label `grpc_code=STATUS_CODE_NAME` — RPCs that finished
  with specified status codes, one metric per gRPC status. Zero `status`
  metrics are omitted, except for `OK` and `UNKNOWN` metrics, which are always
  written.
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

@ref grpc/functional_tests/metrics/tests/static/metrics_values.txt "An example of userver gRPC metrics".


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
