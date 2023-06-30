# gRPC client and service

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

Implement the generated `api::GreeterServiceBase`. As a convenience, a derived `api::GreeterServiceBase::Component` class is provided for easy integration with the component system.

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - service

A single request-response RPC handling is simple: fill in the `response` and send it.

@snippet samples/grpc_service/grpc_service.cpp  gRPC sample - server RPC handling

Fill in the static config entries for the server side:

@snippet samples/grpc_service/static_config.yaml  gRPC sample - static config server

### int main()

Finally, we register our components and start the server.

@snippet samples/http_caching/http_caching.cpp  HTTP caching sample - main

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
@ref md_en_userver_functional_testing "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/grpc_service/userver-samples-grpc_service -c </path/to/static_config.yaml>`
(do not forget to prepare the configuration files!).

The service is available locally at port 8091 (as per our `static_config.yaml`).


### Functional testing
To implement @ref md_en_userver_functional_testing "Functional tests" for the
service some preparational steps should be done.

#### Preparations
First of all, import the required modules and add the required
pytest_userver.plugins.grpc pytest plugin:

@snippet samples/grpc_service/tests/conftest.py  Prepare modules


#### gRPC server mock

To mock the gRPC server provide a hook for the static config to change
the endpoint:

@snippet samples/grpc_service/tests/conftest.py  Prepare configs

Write the mocking fixtures using @ref pytest_userver.plugins.grpc_mockserver.grpc_mockserver "grpc_mockserver":

@snippet samples/grpc_service/tests/conftest.py  Prepare server mock

After that everything is ready to check the service client requests:

@snippet samples/grpc_service/tests/test_grpc.py  grpc client test

#### gRPC client

To do the gRPC requests write a client fixture using
@ref pytest_userver.plugins.grpc_client.grpc_channel "grpc_channel":

@snippet samples/grpc_service/tests/conftest.py  grpc client

Use it to do gRPC requests to the service:

@snippet samples/grpc_service/tests/test_grpc.py  grpc server test


## Full sources

See the full example at:

* @ref samples/grpc_service/grpc_service.cpp
* @ref samples/grpc_service/proto/samples/greeter.proto
* @ref samples/grpc_service/static_config.yaml
* @ref samples/grpc_service/tests/conftest.py
* @ref samples/grpc_service/tests/test_grpc.py
* @ref samples/grpc_service/dynamic_config_fallback.json
* @ref samples/grpc_service/CMakeLists.txt

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_tutorial_flatbuf_service | @ref md_en_userver_tutorial_postgres_service ⇨
@htmlonly </div> @endhtmlonly

@example samples/grpc_service/grpc_service.cpp
@example samples/grpc_service/proto/samples/greeter.proto
@example samples/grpc_service/static_config.yaml
@example samples/grpc_service/tests/conftest.py
@example samples/grpc_service/tests/test_grpc.py
@example samples/grpc_service/dynamic_config_fallback.json
@example samples/grpc_service/CMakeLists.txt
