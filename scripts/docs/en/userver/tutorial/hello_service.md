## Writing your first HTTP server

## Before you start

Make sure that you can compile and run core tests as described at
@ref scripts/docs/en/userver/tutorial/build.md.

Note that there is a ready to use opensource
[service template](https://github.com/userver-framework/service_template)
to ease the development of your userver based services. The template already has
a preconfigured CI, build and install scripts, testsuite and unit-tests setups.

## Step by step guide

Typical HTTP server application in userver consists of the following parts:
* HTTP handler component - main logic of your application
* Static config - startup config that does not change for the whole lifetime of an application
* int main() - startup code

Let's write a simple server that responds with "Hello world!\n" on every request to `/hello` URL.

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

@snippet samples/hello_service/hello_service.cpp  Hello service sample - component

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

@snippet samples/hello_service/hello_service.cpp  Hello service sample - main


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
Hello world!
```

### Functional testing

@ref scripts/docs/en/userver/functional_testing.md "Functional tests" for the service could be
implemented using the @ref service_client "service_client" fixture from
pytest_userver.plugins.core in the
following way:

@snippet samples/hello_service/tests/test_hello.py  Functional test

Do not forget to add the plugin in conftest.py:

@snippet samples/hello_service/tests/conftest.py  registration

## Full sources

See the full example at:
* @ref samples/hello_service/hello_service.cpp
* @ref samples/hello_service/static_config.yaml
* @ref samples/hello_service/CMakeLists.txt
* @ref samples/hello_service/tests/conftest.py
* @ref samples/hello_service/tests/test_hello.py

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/faq.md | @ref scripts/docs/en/userver/tutorial/config_service.md ⇨
@htmlonly </div> @endhtmlonly


@example samples/hello_service/hello_service.cpp
@example samples/hello_service/static_config.yaml
@example samples/hello_service/CMakeLists.txt
@example samples/hello_service/tests/conftest.py
@example samples/hello_service/tests/test_hello.py

