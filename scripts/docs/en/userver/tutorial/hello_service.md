## Writing your first HTTP server


## Before you start

@warning Note that you can start with a ready to use opensourse [service template](scripts/docs/en/userver/build/build.md)
to ease the development of your userver based services. The template already has
a preconfigured CI, build and install scripts, testsuite and unit-tests setups.

@warning @ref service_templates "service template" is an implementation of an HTTP server application from this tutorial.
You do not need to copy parts of code from this tutorial to @ref service_templates "service template" as it already has them.


Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/build/build.md.


## Step by step guide

Typical HTTP server application in userver consists of the following parts:
* Some application logic
* HTTP handler component -  component that ties application logic to HTTP
  handler.
* Static config - startup config that does not change for the whole lifetime of
  an application.
* `int main()` - startup code.

Let's write a simple server that responds with
* `"Hello, unknown user!\n"` on every request to `/hello` URL without `name` argument;
* `"Hello, <name of the user>!\n"` on every request to `/hello` URL with `?name=<name of the user>`.

This sample also contains information on how to add unit tests, benchmarks and
functional tests.


### Application Logic

Our application logic is straightforward:

@include samples/hello_service/src/say_hello.cpp

The "say_hello.hpp" contains a signe function declaration, so that the
implementation details are hidden and the header is lightweight to include:

@include samples/hello_service/src/say_hello.hpp


### HTTP handler component

HTTP handlers must derive from `server::handlers::HttpHandlerBase` and have a name, that
is obtainable at compile time via `kName` variable and is obtainable at runtime via `HandlerName()`.

The primary functionality of the handler should be located in `HandleRequestThrow` function.
Return value of this function is the HTTP response body. If an exception `exc` derived from
`server::handlers::CustomHandlerException` is thrown from the function then the
HTTP response code will be set to `exc.GetCode()` and `exc.GetExternalErrorBody()`
would be used for HTTP response body. Otherwise if an exception `exc` derived from
`std::exception` is thrown from the function then the
HTTP response code will be set to `500`.

@include samples/hello_service/src/hello_handler.hpp

@include samples/hello_service/src/hello_handler.cpp

@warning `Handle*` functions are invoked concurrently on the same instance of the handler class. Use @ref scripts/docs/en/userver/synchronization.md "synchronization primitives" or do not modify shared data in `Handle*`.


### Static config

Now we have to configure the service by providing `task_processors` and
`default_task_processor` options for the components::ManagerControllerComponent
and configuring each component in `components` section:

@include samples/hello_service/static_config.yaml

Note that all the @ref userver_components "components" and
@ref userver_http_handlers "handlers" have their static options additionally
described in docs.


### int main()

Finally, we
add our component to the `components::MinimalServerComponentList()`,
and start the server with static configuration file passed from command line.

@include samples/hello_service/main.cpp


### CMake

The build scripts consist of the following parts:

* Finding the installed userver package:
  @snippet samples/hello_service/CMakeLists.txt  find_userver
* Making an `OBJECTS` target with built sources that are used across unit tests,
  benchmarks and the service itself:
  @snippet samples/hello_service/CMakeLists.txt  objects
* Building the service executable:
  @snippet samples/hello_service/CMakeLists.txt  executable
* Unit tests:
  @snippet samples/hello_service/CMakeLists.txt  unittests
* Benchmarks:
  @snippet samples/hello_service/CMakeLists.txt  benchmarks
* Finally, we add `test` directory as a directory with tests for testsuite:
  @snippet samples/hello_service/CMakeLists.txt  testsuite


### Build and Run

To build the sample, execute the following build steps at the userver root directory:
```
mkdir build_release
cd build_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make userver-samples-hello_service
```

The sample could be started by running
`make start-userver-samples-hello_service`. The command would invoke
@ref scripts/docs/en/userver/functional_testing.md "testsuite start target" that sets proper
paths in the configuration files and starts the service.

To start the service manually run
`./samples/hello_service/userver-samples-hello_service -c </path/to/static_config.yaml>`.

@note Without file path to `static_config.yaml` `userver-samples-hello_service` will look for a file with name `config_dev.yaml`
@note CMake doesn't copy `static_config.yaml` and file from `samples` directory into build directory.

Now you can send a request to your server from another terminal:
```
bash
$ curl 127.0.0.1:8080/hello
Hello, unknown user!
```

### Unit tests

@ref scripts/docs/en/userver/testing.md "Unit tests" could be implemented with one of UTEST macros in the following way:

@snippet samples/hello_service/unittests/say_hello_test.cpp  Unit test

### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the @ref service_client "service_client" fixture from
pytest_userver.plugins.core in the
following way:

@snippet samples/hello_service/testsuite/test_hello.py  Functional test

Do not forget to add the plugin in conftest.py:

@snippet samples/hello_service/testsuite/conftest.py  registration

## Full sources

See the full example at:
* @ref samples/hello_service/src/hello_handler.hpp
* @ref samples/hello_service/src/hello_handler.cpp
* @ref samples/hello_service/src/say_hello.hpp
* @ref samples/hello_service/src/say_hello.cpp
* @ref samples/hello_service/static_config.yaml
* @ref samples/hello_service/main.cpp
* @ref samples/hello_service/CMakeLists.txt
* @ref samples/hello_service/unittests/say_hello_test.cpp
* @ref samples/hello_service/benchmarks/say_hello_bench.cpp
* @ref samples/hello_service/testsuite/conftest.py
* @ref samples/hello_service/testsuite/test_hello.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/build/userver.md | @ref scripts/docs/en/userver/tutorial/config_service.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/hello_service/src/hello_handler.hpp
@example samples/hello_service/src/hello_handler.cpp
@example samples/hello_service/src/say_hello.hpp
@example samples/hello_service/src/say_hello.cpp
@example samples/hello_service/static_config.yaml
@example samples/hello_service/main.cpp
@example samples/hello_service/CMakeLists.txt
@example samples/hello_service/unittests/say_hello_test.cpp
@example samples/hello_service/benchmarks/say_hello_bench.cpp
@example samples/hello_service/testsuite/conftest.py
@example samples/hello_service/testsuite/test_hello.py

