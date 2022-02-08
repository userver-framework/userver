# Writing your first gRPC server

## Before you start

Make sure that you can compile and run core tests and read a basic example @ref
md_en_userver_tutorial_hello_service.

## Step by step guide

In this example, we will write a client side and a server side for a simple `GreeterService` from `greeter.proto` (see the schema below). Its single `SayHello` method accepts a `name` string and replies with a corresponding `greeting` string.

### Installation

We generate and link to a CMake library from our `.proto` schema:

@snippet samples/grpc_service/CMakeLists.txt  gRPC sample - CMake

Register the necessary `ugrpc` components:

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - ugrpc registration

### The client side

Wrap the generated `api::GreeterServiceClient` in a component that exposes a simplified interface:

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - client

A single request-response RPC handling is simple: fill in `request` and `context`, initiate the RPC, receive the `response`.

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - client RPC handling

Fill in the static config entries for the client side:

@snippet samples/grpc_service/static_config.yaml  gRPC sample - static config client

@snippet samples/grpc_service/static_config.yaml  gRPC sample - task processor

### The server side

Implement the generated `api::GreeterServiceBase`. A single request-response RPC handling is simple: fill in the `response` and send it.

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - server RPC handling

Connect the service implementation to the component system:

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - service

Fill in the static config entries for the server side:

@snippet samples/grpc_service/static_config.yaml  gRPC sample - static config server

### int main()

Finally, we register our components and start the server.

@snippet samples/http_caching/http_caching.cpp  HTTP caching sample - main

### Build

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-grpc_service
```

Start the server by running `./samples/http_caching/userver-samples-grpc_service`. The service is available locally at port 8091 (as per our `static_config.yaml`).

## Full sources

See the full example at:

* @ref samples/grpc_service/grpc_service.cpp
* @ref samples/grpc_service/proto/samples/greeter.proto
* @ref samples/grpc_service/static_config.yaml
* @ref samples/grpc_service/dynamic_config_fallback.json
* @ref samples/grpc_service/CMakeLists.txt

@example samples/grpc_service/grpc_service.cpp
@example samples/grpc_service/proto/samples/greeter.proto
@example samples/grpc_service/static_config.yaml
@example samples/grpc_service/dynamic_config_fallback.json
@example samples/grpc_service/CMakeLists.txt
