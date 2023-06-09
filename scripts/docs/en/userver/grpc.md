# gRPC

## Introduction

🐙 **userver** provides a gRPC driver as `userver-grpc` library. It uses ```namespace ugrpc::client``` and ```namespace ugrpc::server```.

The driver wraps `grpcpp` in the userver asynchronous interface.

See also:
* @ref md_en_userver_tutorial_grpc_service
* [Official gRPC documentation](https://grpc.io/docs/languages/cpp/)
* [grpcpp reference](https://grpc.github.io/grpc/cpp/index.html)
* [grpc-core reference](https://grpc.github.io/grpc/core/index.html)

## Capabilities

* Creating asynchronous gRPC clients and services
* Forwarding gRPC Core logs to userver logs
* Caching and reusing connections (**TODO**)
* Timeouts and retries of requests (**TODO**)
* Collection of metrics on driver usage (**TODO**)
* Cancellation support (**TODO**)
* Automatic authentication (**TODO**)

## Installation

Generate and link a library from `.proto` schemas and link to it in your `CMakeLists.txt`:

@snippet samples/grpc_service/CMakeLists.txt  gRPC sample - CMake

`add_grpc_library` will link `userver-grpc` transitively and will generate the usual `.pb.h + .pb.cc` files. For service definitions, it will additionally generate asynchronous interfaces `foo_client.usrv.pb.hpp` and `foo_service.usrv.pb.hpp`.

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

### Middlewares

The gRPC server can be extended by middlewares. Middleware is called on each incoming RPC request. Different middlewares handle the call subsequently. A middleware may decide to reject the call or call the next middleware in the stack. Middlewares may implement almost any enhancement to the gRPC server including authorization and authentication, ratelimiting, logging, tracing, audit, etc.

Middlewares to use are indicated in static config in section `middlewares` of `ugrpc::server::ServiceComponentBase` descendant component.

## Metrics

* Client metrics are put inside `grpc.client.by-destination {grpc_destination=FULL_SERVICE_NAME/METHOD_NAME}`
* Server metrics are put inside `grpc.server.by-destination {grpc_destination=FULL_SERVICE_NAME/METHOD_NAME}`

These are the metrics provided for each gRPC method:

| Metric name             | Description                                                     |
|-------------------------|-----------------------------------------------------------------|
| timings.1min            | time from RPC start to finish (`utils::statistics::Percentile`) |
| status.STATUS_CODE_NAME | RPCs that finished with specified status codes                  |
| network-error           | RPCs that did not finish with a status due to a network error   |
| abandoned-error         | RPCs that we forgot to `Finish` (always a bug in `ugrpc` usage) |
| rps                     | Requests per second: `sum(status) + network-error`              |
| eps                     | Errors per second: `rps - status.OK`                            |
| active                  | The number of currently active RPCs (created and not finished)  |


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_profile_context_switches | @ref rabbitmq_driver ⇨
@htmlonly </div> @endhtmlonly
