# gRPC middleware

## Before you start

Make sure that you can compile and run core tests and read a basic example
@ref scripts/docs/en/userver/tutorial/hello_service.md.

## Step by step guide

In this example, you will write an authentication middleware for both
'GreeterService' and 'GreeterClient' of the basic grpc_service. 
See @ref scripts/docs/en/userver/tutorial/grpc_service.md

### Installation

Generate and link with necessary libraries:

@snippet samples/grpc_middleware_service/CMakeLists.txt  gRPC middleware sample - CMake

### The client middleware

Client middleware will add metadata to every `GreeterClient` call.

Derive `Middleware` and `MiddlewareFactory` from the respective base class:

@snippet samples/grpc_middleware_service/src/middlewares/client/middleware.hpp gRPC middleware sample - Middleware and MiddlewareFactory declaration

Handle method of `Middleware` does the actual work:

@snippet samples/grpc_middleware_service/src/middlewares/client/middleware.cpp gRPC middleware sample - Middleware and MiddlewareFactory implementation

Then, wrap it into component, which just stores `MiddlewareFactory`:

@snippet samples/grpc_middleware_service/src/middlewares/client/component.hpp gRPC middleware sample - Middleware component declaration

Lastly, add this component to the static config:

```
# yaml
components_manager:
    components:
        grpc-auth-client:
```

And connect it with `ClientFactory`:

@snippet samples/grpc_middleware_service/static_config.yaml gRPC middleware sample - static config client middleware


### The server middleware

Server middleware, in its turn, will validate metadata that comes with an rpc.

Everything is the same as it is for client middleware, except there is no
factory and the component stores the middleware itself:

@snippet samples/grpc_middleware_service/src/middlewares/server/middleware.hpp gRPC middleware sample - Middleware declaration

Handle method of `Middleware` does the actual work:

@snippet samples/grpc_middleware_service/src/middlewares/server/middleware.cpp gRPC middleware sample - Middleware implementation

Respective component:

@snippet samples/grpc_middleware_service/src/middlewares/server/component.hpp gRPC middleware sample - Middleware component declaration

Lastly, add this component to the static config:

```
# yaml
components_manager:
    components:
        grpc-auth-server:
```

And connect it with `Service`:

@snippet samples/grpc_middleware_service/static_config.yaml gRPC middleware sample - static config server middleware


### int main()

Finally, register components and start the server.

@snippet samples/grpc_middleware_service/src/main.cpp gRPC middleware sample - components registration


### Build and Run

To build the sample, execute the following build steps at the userver root
directory:

```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-grpc_middleware_service
```

The sample could be started by running
`make start-userver-samples-grpc_middleware_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/grpc_middleware_service/userver-samples-grpc_middleware_service -c </path/to/static_config.yaml>`.

The service is available locally at port 8091 (as per project `static_config.yaml`).


### Functional testing
To implement @ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the
service some preparational steps should be done.


#### Preparations
First of all, import the required modules and add the required
pytest_userver.plugins.grpc pytest plugin:

@snippet samples/grpc_middleware_service/tests/conftest.py  Prepare modules


#### gRPC server mock

To mock the gRPC server provide a hook for the static config to change
the endpoint:

@snippet samples/grpc_middleware_service/tests/conftest.py  Prepare configs

Write the mocking fixtures using @ref pytest_userver.plugins.grpc_mockserver.grpc_mockserver "grpc_mockserver":

@snippet samples/grpc_middleware_service/tests/conftest.py  Prepare server mock


#### gRPC client

To do the gRPC requests write a client fixture using
@ref pytest_userver.plugins.grpc_client.grpc_channel "grpc_channel":

@snippet samples/grpc_middleware_service/tests/conftest.py  grpc client

Use it to do gRPC requests to the service:

@snippet samples/grpc_middleware_service/tests/test_middlewares.py  grpc authentication tests


## Full sources

See the full example at:

* @ref samples/grpc_middleware_service/src/middlewares/client/middleware.hpp
* @ref samples/grpc_middleware_service/src/middlewares/client/middleware.cpp
* @ref samples/grpc_middleware_service/src/middlewares/client/component.hpp
* @ref samples/grpc_middleware_service/src/middlewares/client/component.cpp

* @ref samples/grpc_middleware_service/src/middlewares/server/middleware.hpp
* @ref samples/grpc_middleware_service/src/middlewares/server/middleware.cpp
* @ref samples/grpc_middleware_service/src/middlewares/server/component.hpp
* @ref samples/grpc_middleware_service/src/middlewares/server/component.cpp

* @ref samples/grpc_middleware_service/src/middlewares/auth.hpp
* @ref samples/grpc_middleware_service/src/middlewares/auth.cpp

* @ref samples/grpc_middleware_service/src/main.cpp
* @ref samples/grpc_middleware_service/proto/samples/greeter.proto
* @ref samples/grpc_middleware_service/static_config.yaml
* @ref samples/grpc_middleware_service/tests/conftest.py
* @ref samples/grpc_middleware_service/tests/test_middlewares.py
* @ref samples/grpc_middleware_service/CMakeLists.txt

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/tutorial/grpc_service.md | @ref scripts/docs/en/userver/tutorial/postgres_service.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/grpc_middleware_service/src/middlewares/client/middleware.hpp
@example samples/grpc_middleware_service/src/middlewares/client/middleware.cpp
@example samples/grpc_middleware_service/src/middlewares/client/component.hpp
@example samples/grpc_middleware_service/src/middlewares/client/component.cpp
@example samples/grpc_middleware_service/src/middlewares/server/middleware.hpp
@example samples/grpc_middleware_service/src/middlewares/server/middleware.cpp
@example samples/grpc_middleware_service/src/middlewares/server/component.hpp
@example samples/grpc_middleware_service/src/middlewares/server/component.cpp
@example samples/grpc_middleware_service/src/middlewares/auth.hpp
@example samples/grpc_middleware_service/src/middlewares/auth.cpp
@example samples/grpc_middleware_service/src/main.cpp
@example samples/grpc_middleware_service/proto/samples/greeter.proto
@example samples/grpc_middleware_service/static_config.yaml
@example samples/grpc_middleware_service/tests/conftest.py
@example samples/grpc_middleware_service/tests/test_middlewares.py
@example samples/grpc_middleware_service/CMakeLists.txt
