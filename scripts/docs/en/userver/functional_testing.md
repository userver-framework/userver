# Functional service tests (testsuite)

## Getting started

Userver has built-in support for functional service tests using [Yandex.Taxi Testsuite](https://pypi.org/project/yandex-taxi-testsuite/).
Testsuite is based on [pytest](https://pytest.org/) and allows developers to test thier services
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

## CMake integration

With `userver_testsuite_add()` function you can easily add testsuite support to your project.
Its main purpose is:

* Setup Python environment `virtualenv` or  uses existing one.
* Create runner script that setups `PYTHONPATH` and passes extra arguments to `pytest`.
* Registers `ctest` target.


@ref cmake/UserverTestsuite.cmake library is automatically addded to CMake path
after userever enviroment setup. Add the following line to use it:

@snippet testsuite/SetupUserverTestsuiteEnv.cmake testsuite - UserverTestsuite

Then create testsuite target:
@snippet samples/testsuite-support/CMakeLists.txt testsuite - cmake

### Arguments

* NAME, required test name used as `ctest` target name.
* WORKING_DIRECTORY, pytest working directory. Default is ${CMAKE_CURRENT_SOURCE_DIR}.
* PYTEST_ARGS, list of extra arguments passed to `pytest`.
* PYTHONPATH, list of directories to be prepended to `PYTHONPATH`.
* REQUIREMENTS, list of reqirements.txt files used to populate `virtualenv`.
* PYTHON_BINARY, path to existing Python binary.
* VIRTUALENV_ARGS, list of extra arguments passed to `virtualenv`.

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

`${CMAKE_CURRENT_BINARY_DIR}/venv-${ARG_NAME}`

### Run with ctest

`userver_testsuite_add()` registers a ctest target with name NAME.
Run all project tests with ctest command or use filters to run specific tests:

```shell
ctest -V -R my-project  # NAME argument is used
```

### Direct run

To run pytest directly `userver_testsuite_add()` creates a testsuite runner script
that could be found in corresponding binary directory.
This may be useful to run a single testcase, to start the testsuite with gdb or to
start the testsuite with extra pytest arguments:

`${CMAKE_CURRENT_BINARY_DIR}/runtests-${ARG_NAME}`

You can use it to manually start testsuite with extra `pytest` arguments, e.g.:


```shell
./build/tests/runtests-my-project -vvx ./tests -k test_foo
```

Please refer to `testuite` and `pytest` documentation for available options.
Run it with `--help` argument to see the short options description.

```shell
./build/tests/runtests-my-project ./tests --help
```

## pytest_userver

By default internal `pytest_userver` plugin is included in python path.
It provides basic testsuite support for userver service.
To use it add it to your `pytest_plugins` in root `conftest.py`:

@snippet samples/testsuite-support/tests/conftest.py testsuite - pytest_plugins

It requires extra PYTEST_ARGS to be passed:

@snippet samples/testsuite-support/CMakeLists.txt testsuite - cmake


### Userver testsuite support

Userver has built-in support for testsuite.

In order to use it you need to register correspoding components:

Headers:

@snippet samples/testsuite-support/main.cpp testsuite - include components

Add components to components list:

@snippet samples/testsuite-support/main.cpp testsuite - register components

Add testsuite components to `config.yaml`:

@snippet samples/testsuite-support/static_config.yaml testsuite - config.yaml

@warning Please note that the testsuite support must be disabled in production enviroment.
Testsuite sets the `testsuite-enabled` variable into `true` when runs the service.
In the example above this variable controls whether or `tests-control` component is loaded.

### Features

#### Service config generation

`pytest_userver` uses config.yaml and config_vars.yaml passed to pytest to generate
services configs.
`pytest_userver` provides a way to modify this configs before startings service.
You can decalre `USERVER_CONFIG_HOOKS` variable in your pytest-plugin it is list of
functions or pytest-fixtures that are run before config is written to disk.
Example usage:

@snippet samples/production_service/tests/conftest.py config hook

#### Service client

Fixture `service_client` is used to access service being tested:

@snippet samples/testsuite-support/tests/test_ping.py service_client

When `tests-control` component is enabled service_client is
`pytest_userver.client.Client` instance.
Which supports special testsuite related methods.

On first call to `service_client` service state is implicitly updated, e.g.:
caches, mocked time, etc.

#### Service environment variables

Use this fixture to provide extra environment variables for your service:

@snippet samples/redis_service/tests/conftest.py service_env

#### Extra client dependencies

Use this fixture to provide extra fixtures that your service depends on:

@snippet samples/postgres_service/tests/conftest.py client_deps

#### Mockserver

[Mockserver](https://yandex.github.io/yandex-taxi-testsuite/mockserver/) allows to mock external
HTTP handlers. It starts its own HTTP server that receives HTTP traffic from service being tested.
And allows to install custom HTTP handlers within testsuite.
In order to use it all HTTP clients must be pointed to mockserver address.

Mockserver usage example:

@snippet samples/http_caching/tests/conftest.py mockserver

* Testcase: @ref samples/http_caching/tests/conftest.py

#### Mock time

Userver provides a way to mock internal datetime value. It only works for datetime retrieved
with `utils::datetime::Now()`, see [Mocked time section](#utest-mocked-time) for details.

From testsuite you can control it with [mocked_time](https://yandex.github.io/yandex-taxi-testsuite/other_plugins/#mocked-time)
plugin.

Example usage:

@snippet samples/testsuite-support/tests/test_mocked_time.py mocked_time

Example are available here:

* C++ code: @ref samples/testsuite-support/now.cpp
* Testcase: @ref samples/testsuite-support/tests/test_mocked_time.py

#### Testpoint

[Testpoints](https://yandex.github.io/yandex-taxi-testsuite/testpoint/) are used to send messages
from the service to testcase and back. Typical use cases are:

* Retrieve intermediate state of the service and test it
* Inject errors into the service
* Synchronize service and testcase execution

First of all you should include testpoint header:

@snippet samples/testsuite-support/testpoint.cpp Testpoint - include

It provides `TESTPOINT()` and family of `TESTPOINT_CALLBACK()` macroses that do nothing in
production environment and only work when run under testsuite.
Under testsuite they only make sense when corresponding testsuite handler is installed.

All testpoints has their own name that is used to call named testsuite handler.
Second argument is `formats::json::Value()` instance that is only evaluated under testsuite.

`TESTPOINT()` usage sample:

@snippet samples/testsuite-support/testpoint.cpp Testpoint - TESTPOINT()

Then you can use testpoint from testcase:

@snippet samples/testsuite-support/tests/test_testpoint.py Testpoint - fixture

In order to eliminate unnecessary testpoint requests userver keeps track of testpoints
that have testsuite handlers installed. Usually testpoint handlers are declared before
first call to `service_client` which implicitly updates userver's list of testpoint.
Sometimes it might be required to manually update server state.
This can be achieved using `service_client.update_server_state()` method e.g.:

@snippet samples/testsuite-support/tests/test_testpoint.py Manual registration

Accessing testpoint userver is not aware of will raise an exception:

@snippet samples/testsuite-support/tests/test_testpoint.py Unregistred testpoint usage

* C++ code: @ref samples/testsuite-support/testpoint.cpp
* Testcase: @ref samples/testsuite-support/tests/test_testpoint.py

#### Logs capture

Testsuite can be used to test logs written by service.
To achieve this the testsuite starts a simple logs capture TCP server and
tells the service to replicate logs to it on per test basis.

Example usage:
@snippet samples/testsuite-support/tests/test_logcapture.py select

Example on logs capture usage could be found here:

* C++ code: @ref samples/testsuite-support/logcapture.cpp
* Testcase: @ref samples/testsuite-support/tests/test_logcapture.py

#### Testsuite tasks

Testsuite tasks facility allows to register a custom function and call it by name from testsuite.
It's useful for testing components that perform periodic job not related to its own HTTP handler.

You can use `testsuite::TestsuiteTasks` to register your own task:

@snippet samples/testsuite-support/tasks.cpp register

After that you can call your task from testsuite code:

@snippet samples/testsuite-support/tests/test_tasks.py run_task

Or spawn the task asynchronously using context manager:

@snippet samples/testsuite-support/tests/test_tasks.py spawn_task

An example on testsuite tasks could be found here:

* C++ code: @ref samples/testsuite-support/tasks.cpp
* Testcase: @ref samples/testsuite-support/tests/test_tasks.py

#### Metrics

Testsuite provides access to userver metrics, see @ref tutorial_metrics "tutorial on configuration".
It allows to:

- retrieve service metrics with `await monitor_client.get_metrics()`
- reset metrics using `await service_client.reset_metrics()`

Example usage:

@snippet samples/testsuite-support/tests/test_metrics.py metrics reset

* C++ code: @ref samples/testsuite-support/metrics.cpp
* C++ header: @ref samples/testsuite-support/metrics.hpp
* Testcase: @ref samples/testsuite-support/tests/test_metrics.py

@example cmake/UserverTestsuite.cmake
@example samples/http_caching/tests/conftest.py
@example samples/testsuite-support/logcapture.cpp
@example samples/testsuite-support/metrics.cpp
@example samples/testsuite-support/metrics.hpp
@example samples/testsuite-support/now.cpp
@example samples/testsuite-support/tasks.cpp
@example samples/testsuite-support/testpoint.cpp
@example samples/testsuite-support/tests/test_logcapture.py
@example samples/testsuite-support/tests/test_metrics.py
@example samples/testsuite-support/tests/test_mocked_time.py
@example samples/testsuite-support/tests/test_tasks.py
@example samples/testsuite-support/tests/test_testpoint.py
