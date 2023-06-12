# Functional service tests (testsuite)

## Getting started

🐙 **userver** has built-in support for functional service tests using
[Yandex.Taxi Testsuite](https://pypi.org/project/yandex-taxi-testsuite/).
Testsuite is based on [pytest](https://pytest.org/) and allows developers to test their services
in isolated environment.
It starts service binary with minimal database and all external services mocked then allows
developer to call service handlers and test their result.

Supported features:

* Database startup (Mongo, Postgresql, Clickhouse, ...)
* Per-test database state
* Service startup
* Mocksever to mock external service handlers
* Mock service time, `utils::datetime::Now()`
* Testpoint
* Cache invalidation
* Logs capture
* Service runner

## CMake integration

With `userver_testsuite_add()` function you can easily add testsuite support to your project.
Its main purpose is:

* Setup Python environment `virtualenv` or use an existing one.
* Create runner script that setups `PYTHONPATH` and passes extra arguments to `pytest`.
* Registers `ctest` target.
* Adds a `start-*` target that starts the service and databases with testsuite
  configs and waits for keyboard interruption to stop the service.


@ref cmake/UserverTestsuite.cmake library is automatically added to CMake path
after userever environment setup. Add the following line to use it:

@snippet testsuite/SetupUserverTestsuiteEnv.cmake testsuite - UserverTestsuite

Then create testsuite target:
@snippet samples/testsuite-support/CMakeLists.txt testsuite - cmake

### Arguments

* SERVICE_TARGET, required CMake name of the target service to test. Used as
  suffix for `testsuite-` and `start-` CMake target names.
* WORKING_DIRECTORY, pytest working directory. Default is ${CMAKE_CURRENT_SOURCE_DIR}.
* PYTEST_ARGS, list of extra arguments passed to `pytest`.
* PYTHONPATH, list of directories to be prepended to `PYTHONPATH`.
* REQUIREMENTS, list of reqirements.txt files used to populate `virtualenv`.
* PYTHON_BINARY, path to existing Python binary.
* VIRTUALENV_ARGS, list of extra arguments passed to `virtualenv`.
* PRETTY_LOGS, set to `OFF` to disable pretty printing.

Some of the most useful arguments for PYTEST_ARGS:

| Argument                        | Description                             |
|---------------------------------|-----------------------------------------|
| `-v`                            | Increase verbosity                      |
| `-s`                            | Do not intercept `stdout` and `stderr`  |
| `-x`                            | Stop on first error                     |
| `-k EXPRESSION`                 | Filter tests by expression              |
| `--service-logs-pretty`         | Enable logs coloring                    |
| `--service-logs-pretty-disable` | Disable logs coloring                   |
| `--service-log-level=LEVEL`     | Set the log level for the service. Possible values: `trace`, `debug`, `info`, `warning`, `error`, `critical` |
| `--service-wait`                | With this argument the testsuite will wait for the service start by user. For example under gdb. Testsuite outputs a hint on starting the service |
| `-rf`                           | Show a summary of failed tests          |


### Python environment

You may want to create new virtual environment with its own set of packages. Or reuse existing one.
That could be done this way:

- If REQUIREMENTS is given then new virtualenv is created
- Else if PYTHON_BINARY is specified it's used
- Otherwise value of ${TESTSUITE_VENV_PYTHON} is used

Basic `requirements.txt` file may look like this:

```
yandex-taxi-testsuite[mongodb]
```

Creating per-testsuite virtual environment is a recommended way to go.
It creates virtualenv that could be found in current binary directory:

`${CMAKE_CURRENT_BINARY_DIR}/venv-testsuite-${SERVICE_TARGET}`

### Run with ctest

`userver_testsuite_add()` registers a ctest target with name `testsuite-${SERVICE_TARGET}`.
Run all project tests with ctest command or use filters to run specific tests:

```shell
ctest -V -R testsuite-my-project  # SERVICE_TARGET argument is used
```

### Direct run

To run pytest directly `userver_testsuite_add()` creates a testsuite runner script
that could be found in corresponding binary directory.
This may be useful to run a single testcase, to start the testsuite with gdb or to
start the testsuite with extra pytest arguments:

`${CMAKE_CURRENT_BINARY_DIR}/runtests-testsuite-${SERVICE_TARGET}`

You can use it to manually start testsuite with extra `pytest` arguments, e.g.:


```shell
./build/tests/runtests-testsuite-my-project -vvx ./tests -k test_foo
```

Please refer to `testuite` and `pytest` documentation for available options.
Run it with `--help` argument to see the short options description.

```shell
./build/tests/runtests-testsuite-my-project ./tests --help
```

### Debug 

To debug the functional test you can start testsuite with extra `pytest` arguments, e.g.:


```shell
./build/tests/runtests-testsuite-my-project --service-wait ./tests -k test_foo
```

At the beginning of the execution the console will display the command to start the service, e.g.:


```shell
gdb --args /.../my-project/build/functional-tests --config /.../config.yaml
```

Now you can open a new terminal window and run this command in it or if 
you use an IDE you can find the corresponding CMake target and add arg `--config /.../config.yaml`.
After that it will be possible to set breakpoints and start target with debug.

## pytest_userver

By default `pytest_userver` plugin is included in python path.
It provides basic testsuite support for userver service.
To use it add it to your `pytest_plugins` in root `conftest.py`:

@snippet samples/testsuite-support/tests/conftest.py testsuite - pytest_plugins

It requires extra PYTEST_ARGS to be passed:

@snippet samples/testsuite-support/CMakeLists.txt testsuite - cmake

The plugins match the userver cmake targets. For example, if the service links
with `userver-core` its tests should use the pytest_userver.plugins.core
plugin.

| CMake target       | Matching plugin for testsuite     |
|--------------------|-----------------------------------|
| userver-core       | pytest_userver.plugins.core       |
| userver-grpc       | pytest_userver.plugins.grpc       |
| userver-postgresql | pytest_userver.plugins.postgresql |
| userver-clickhouse | pytest_userver.plugins.clickhouse |
| userver-redis      | pytest_userver.plugins.redis      |
| userver-mongo      | pytest_userver.plugins.mongo      |
| userver-rabbitmq   | pytest_userver.plugins.rabbitmq   |
| userver-mysql      | pytest_userver.plugins.mysql      |


### Userver testsuite support

Userver has built-in support for testsuite.

In order to use it you need to register corresponding components:

Headers:

@snippet samples/testsuite-support/src/main.cpp testsuite - include components

Add components to components list:

@snippet samples/testsuite-support/src/main.cpp testsuite - register components

Add testsuite components to `config.yaml`:

@snippet samples/testsuite-support/static_config.yaml testsuite - config.yaml

@warning Please note that the testsuite support must be disabled in production environment.
Testsuite sets the `testsuite-enabled` variable into `true` when runs the service.
In the example above this variable controls whether or `tests-control` component is loaded.

### Features

The essential parts of the testsuite are
@ref service_client "pytest_userver.plugins.service_client.service_client" and
pytest_userver.plugins.service_client.monitor_client fixtures that give you
access to the pytest_userver.client.Client and
pytest_userver.client.ClientMonitor respectively. Those types allow to interact
with a running service.

Testsuite functions reference could be found at @ref userver_testsuite.

@anchor SERVICE_CONFIG_HOOKS
#### Service config generation

`pytest_userver` modifies static configs `config.yaml` and `config_vars.yaml`
passed to pytest before starting the userver based service.

To apply additional modifications to the static config files
declare `USERVER_CONFIG_HOOKS` variable in your pytest-plugin with a list of
functions or pytest-fixtures that should be run before config is written to disk.
USERVER_CONFIG_HOOKS values are collected across different files and all the
collected functions and fixtures are applied.

Example usage:

@snippet samples/grpc_service/tests/conftest.py Prepare configs

#### Service client

Fixture @ref "service_client"
is used to access the service being tested:

@snippet samples/testsuite-support/tests/test_ping.py service_client

When `tests-control` component is enabled service_client is
`pytest_userver.client.Client` instance.
Which supports special testsuite related methods.

On first call to `service_client` service state is implicitly updated, e.g.:
caches, mocked time, etc.

#### Service environment variables

Use @ref pytest_userver.plugins.service.service_env "service_env" fixture
to provide extra environment variables for your service:

@snippet samples/redis_service/tests/conftest.py service_env

#### Extra client dependencies

Use @ref pytest_userver.plugins.service_client.extra_client_deps "extra_client_deps"
fixture to provide extra fixtures that your service depends on:

@code{.py}
@pytest.fixture
def extra_client_deps(some_fixture_that_required_by_service, some_other_fixture):
    pass
@endcode

Note that @ref pytest_userver.plugins.service_client.auto_client_deps "auto_client_deps"
fixture already knows about the userver supported databases and clients, so
usually you do not need to manually register any dependencies.

#### Mockserver

[Mockserver](https://yandex.github.io/yandex-taxi-testsuite/mockserver/) allows to mock external
HTTP handlers. It starts its own HTTP server that receives HTTP traffic from
the service being tested.
And allows to install custom HTTP handlers within testsuite.
In order to use it all HTTP clients must be pointed to mockserver address.

Mockserver usage example:

@snippet samples/http_caching/tests/conftest.py mockserver

* Testcase: @ref samples/http_caching/tests/conftest.py

To connect your HTTP client to the mockserver make the HTTP client use a base
URL of the form **http://{mockserver}/{service_name}/**.

This could be achieved by patching static config as described in
@ref SERVICE_CONFIG_HOOKS "config hooks" and providing a mockserver address using
[mockserver_info.url(path)](https://yandex.github.io/yandex-taxi-testsuite/mockserver/#testsuite.mockserver.classes.MockserverInfo.url):

@snippet samples/http_caching/tests/conftest.py patch configs

#### Mock time

Userver provides a way to mock internal datetime value. It only works for datetime retrieved
with `utils::datetime::Now()`, see [Mocked time section](#utest-mocked-time) for details.

From testsuite you can control it with [mocked_time](https://yandex.github.io/yandex-taxi-testsuite/other_plugins/#mocked-time)
plugin.

Example usage:

@snippet samples/testsuite-support/tests/test_mocked_time.py mocked_time

Example are available here:

* C++ code: @ref samples/testsuite-support/src/now.cpp
* Testcase: @ref samples/testsuite-support/tests/test_mocked_time.py

#### Testpoint

[Testpoints](https://yandex.github.io/yandex-taxi-testsuite/testpoint/) are used to send messages
from the service to testcase and back. Typical use cases are:

* Retrieve intermediate state of the service and test it
* Inject errors into the service
* Synchronize service and testcase execution

First of all you should include testpoint header:

@snippet samples/testsuite-support/src/testpoint.cpp Testpoint - include

It provides `TESTPOINT()` and family of `TESTPOINT_CALLBACK()` macros that do nothing in
production environment and only work when run under testsuite.
Under testsuite they only make sense when corresponding testsuite handler is installed.

All testpoints has their own name that is used to call named testsuite handler.
Second argument is `formats::json::Value()` instance that is only evaluated under testsuite.

`TESTPOINT()` usage sample:

@snippet samples/testsuite-support/src/testpoint.cpp Testpoint - TESTPOINT()

Then you can use testpoint from testcase:

@snippet samples/testsuite-support/tests/test_testpoint.py Testpoint - fixture

In order to eliminate unnecessary testpoint requests userver keeps track of testpoints
that have testsuite handlers installed. Usually testpoint handlers are declared before
first call to @ref service_client which implicitly updates userver's list of testpoint.
Sometimes it might be required to manually update server state.
This can be achieved using `service_client.update_server_state()` method e.g.:

@snippet samples/testsuite-support/tests/test_testpoint.py Manual registration

Accessing testpoint userver is not aware of will raise an exception:

@snippet samples/testsuite-support/tests/test_testpoint.py Unregistered testpoint usage

* C++ code: @ref samples/testsuite-support/src/testpoint.cpp
* Testcase: @ref samples/testsuite-support/tests/test_testpoint.py

#### Logs capture

Testsuite can be used to test logs written by service.
To achieve this the testsuite starts a simple logs capture TCP server and
tells the service to replicate logs to it on per test basis.

Example usage:
@snippet samples/testsuite-support/tests/test_logcapture.py select

Example on logs capture usage could be found here:

* C++ code: @ref samples/testsuite-support/src/logcapture.cpp
* Testcase: @ref samples/testsuite-support/tests/test_logcapture.py

@anchor TESTSUITE_TASKS
#### Testsuite tasks

Testsuite tasks facility allows to register a custom function and call it by name from testsuite.
It's useful for testing components that perform periodic job not related to its own HTTP handler.

You can use `testsuite::TestsuiteTasks` to register your own task:

@snippet samples/testsuite-support/src/tasks.cpp register

After that you can call your task from testsuite code:

@snippet samples/testsuite-support/tests/test_tasks.py run_task

Or spawn the task asynchronously using context manager:

@snippet samples/testsuite-support/tests/test_tasks.py spawn_task

An example on testsuite tasks could be found here:

* C++ code: @ref samples/testsuite-support/src/tasks.cpp
* Testcase: @ref samples/testsuite-support/tests/test_tasks.py

@anchor TESTSUITE_METRICS_TESTING
#### Metrics

Testsuite provides access to userver metrics written by
utils::statistics::Writer via
@ref pytest_userver.plugins.service_client.monitor_client "monitor_client"
, see @ref tutorial_metrics "tutorial on configuration".
It allows to:

- retrieve specific service metric by path and (optionally) labels:
  @ref pytest_userver.client.ClientMonitor.single_metric "await monitor_client.single_metric(path, labels)"
- retrieve array of metrics by path prefix and (optionally) labels:
  @ref pytest_userver.client.ClientMonitor.metrics "await monitor_client.metrics(path_prefix, labels)"
- retrieve specific service metric by path and (optionally) labels or `None` if no such metric:
  @ref pytest_userver.client.ClientMonitor.single_metric "await monitor_client.single_metric_optional(path, labels)"
- reset metrics: @ref pytest_userver.client.Client.reset_metrics "await service_client.reset_metrics()"

Example usage:

For a metric tag that is defined as:

@snippet samples/testsuite-support/src/metrics.cpp metrics definition

and used like:

@snippet samples/testsuite-support/src/metrics.cpp metrics usage

the metrics could be retrieved and reset as follows:
 
@snippet samples/testsuite-support/tests/test_metrics.py metrics reset

For metrics with labels, they could be retrieved in the following way: 

@snippet samples/testsuite-support/tests/test_metrics.py metrics labels

The @ref pytest_userver.metrics.Metric "Metric" python type is hashable and
comparable:

@snippet testsuite/tests/test_metrics.py  values set

* C++ code: @ref samples/testsuite-support/src/metrics.cpp
* C++ header: @ref samples/testsuite-support/src/metrics.hpp
* Testcase: @ref samples/testsuite-support/tests/test_metrics.py


#### Metrics Portability

Different monitoring systems and time series databases have different
limitations. To make sure that the metrics of your service could be used on
most of the popular systems, there is special action in
server::handlers::TestsControl.

To use it you could just write the following test:

@code{.py}
async def test_metrics_portability(service_client):
    warnings = await service_client.metrics_portability()
    assert not warnings
@endcode

Note that warnings are grouped by type, so you could check only for
some particular warnings or skip some of them. For example:

@snippet samples/production_service/tests/test_production.py metrics partial portability

* Testcase: @ref samples/production_service/tests/test_production.py


#### Service runner

Testsuite provides a way to start standalone service with all mocks and database started.
This can be done by adding `--service-runner-mode` flag to pytest, e.g.:

```shell
./build/tests/runtests-my-project ./tests -s --service-runner-mode
```

Please note that `-s` flag is required to disable console output capture.

`pytest_userver` provides default service runner testcase.
In order to override it you have to add your own testcase with `@pytest.mark.servicetest`:


@code{.py}
@pytest.mark.servicetest
def test_service(service_client):
    ...
@endcode

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_testing | @ref md_en_userver_chaos_testing ⇨
@htmlonly </div> @endhtmlonly

@example cmake/UserverTestsuite.cmake
@example samples/http_caching/tests/conftest.py
@example samples/testsuite-support/src/logcapture.cpp
@example samples/testsuite-support/src/metrics.cpp
@example samples/testsuite-support/src/metrics.hpp
@example samples/testsuite-support/src/now.cpp
@example samples/testsuite-support/src/tasks.cpp
@example samples/testsuite-support/src/testpoint.cpp
@example samples/testsuite-support/tests/test_logcapture.py
@example samples/testsuite-support/tests/test_metrics.py
@example samples/testsuite-support/tests/test_mocked_time.py
@example samples/testsuite-support/tests/test_tasks.py
@example samples/testsuite-support/tests/test_testpoint.py
@example samples/production_service/tests/test_production.py
