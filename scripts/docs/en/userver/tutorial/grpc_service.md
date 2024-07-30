# gRPC client and service

## Before you start

Make sure that you can compile and run core tests and read a basic example @ref scripts/docs/en/userver/tutorial/hello_service.md.

Make sure that you understand the basic concepts of @ref scripts/docs/en/userver/grpc.md "userver grpc driver".

## Step by step guide

In this example, we will write a client side and a server side for a simple `GreeterService` from `greeter.proto` (see the schema below). Its single `SayHello` method accepts a `name` string and replies with a corresponding `greeting` string.

### Installation

Find and link to userver gRPC:

@snippet samples/grpc_service/CMakeLists.txt  ugrpc

Generate and link to a CMake library from our `.proto` schema:

@snippet samples/grpc_service/CMakeLists.txt  add_grpc_library

By default, `userver_add_grpc_library` looks in `${CMAKE_CURRENT_SOURCE_DIR}/proto`, you can override this using the `SOURCE_PATH` option.

Proto includes can be specified in `INCLUDE_DIRECTORIES` option (multiple directories can be specified).

### The client side

Wrap the generated `api::GreeterServiceClient` in a component that exposes a simplified interface:

@snippet samples/grpc_service/src/greeter_client.hpp  includes

@snippet samples/grpc_service/src/greeter_client.hpp  client

@snippet samples/grpc_service/src/greeter_client.hpp  component

@snippet samples/grpc_service/src/greeter_client.cpp  component

We intentionally split `GreeterClient` from `GreeterClientComponent`
to make the logic unit-testable. If you don't need gtest tests,
you can put the logic into the component directly.

A single request-response RPC handling is simple: fill in `request` and `context`, initiate the RPC, receive the `response`.

@snippet samples/grpc_service/src/greeter_client.cpp  client

Fill in the static config entries for the client side:

@snippet samples/grpc_service/static_config.yaml  static config client

@snippet samples/grpc_service/static_config.yaml  task processor

### The server side

Implement the generated `api::GreeterServiceBase`.
As a convenience, `api::GreeterServiceBase::Component` base class is provided
that inherits from both
`api::GreeterServiceBase` and `ugrpc::server::ServiceComponentBase`.
However, for in this example we will also test our service using gtest, so we
need to split the logic from the component.

@snippet samples/grpc_service/src/greeter_service.hpp  includes

@snippet samples/grpc_service/src/greeter_service.hpp  service

@snippet samples/grpc_service/src/greeter_service.hpp  component

@snippet samples/grpc_service/src/greeter_service.cpp  component

A single request-response RPC handling is simple: fill in the `response` and send it.

@snippet samples/grpc_service/src/greeter_service.cpp  server RPC handling

Fill in the static config entries for the server side:

@snippet samples/grpc_service/static_config.yaml  static config server

### int main()

Finally, we register our components and start the server.

@snippet samples/grpc_service/main.cpp  main

### Build and Run

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-grpc_service
```

The sample could be started by running
`make start-userver-samples-grpc_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/grpc_service/userver-samples-grpc_service -c </path/to/static_config.yaml>`.

The service is available locally at port 8091 (as per our `static_config.yaml`).


### Functional testing for the sample gRPC service and client

To implement @ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the
service some preparational steps should be done.

#### Preparations

First of all, import the required modules and add the required
pytest_userver.plugins.grpc pytest plugin:

@snippet samples/grpc_service/testsuite/conftest.py  Prepare modules


#### gRPC server mock

To mock the gRPC server provide a hook for the static config to change
the endpoint:

@snippet samples/grpc_service/testsuite/conftest.py  Prepare configs

Write the mocking fixtures using @ref pytest_userver.plugins.grpc_mockserver.grpc_mockserver "grpc_mockserver":

@snippet samples/grpc_service/testsuite/conftest.py  Prepare server mock

After that everything is ready to check the service client requests:

@snippet samples/grpc_service/testsuite/test_grpc.py  grpc client test

#### gRPC client

To do the gRPC requests write a client fixture using
@ref pytest_userver.plugins.grpc_client.grpc_channel "grpc_channel":

@snippet samples/grpc_service/testsuite/conftest.py  grpc client

Use it to do gRPC requests to the service:

@snippet samples/grpc_service/testsuite/test_grpc.py  grpc server test


### Unit testing for the sample gRPC service and client (gtest)

First, link the unit tests to `userver::grpc-utest`:

@snippet samples/grpc_service/CMakeLists.txt  gtest

Create a fixture that sets up the gRPC service in unit tests:

@snippet samples/grpc_service/unittests/greeter_service_test.cpp  service fixture

Finally, we can create gRPC service and client in unit tests:

@snippet samples/grpc_service/unittests/greeter_service_test.cpp  service tests

We can also use toy test-only gRPC services for unit tests:

@snippet samples/grpc_service/unittests/greeter_service_test.cpp  client tests


## Full sources

See the full example at:

* @ref samples/grpc_service/proto/samples/greeter.proto
* @ref samples/grpc_service/src/greeter_client.hpp
* @ref samples/grpc_service/src/greeter_client.cpp
* @ref samples/grpc_service/src/greeter_service.hpp
* @ref samples/grpc_service/src/greeter_service.cpp
* @ref samples/grpc_service/main.cpp
* @ref samples/grpc_service/static_config.yaml
* @ref samples/grpc_service/tests/conftest.py
* @ref samples/grpc_service/tests/test_grpc.py
* @ref samples/grpc_service/CMakeLists.txt

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/flatbuf_service.md | @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/grpc_service/proto/samples/greeter.proto
@example samples/grpc_service/src/greeter_client.hpp
@example samples/grpc_service/src/greeter_client.cpp
@example samples/grpc_service/src/greeter_service.hpp
@example samples/grpc_service/src/greeter_service.cpp
@example samples/grpc_service/main.cpp
@example samples/grpc_service/grpc_service.cpp
@example samples/grpc_service/static_config.yaml
@example samples/grpc_service/tests/conftest.py
@example samples/grpc_service/tests/test_grpc.py
@example samples/grpc_service/CMakeLists.txt
