# Testing userver

This page describes tools for testing userver-based services.

## Unit tests (googletest)

### Getting started

All userver test helpers live in `utest` CMake target:

```cmake
target_link_libraries(your-test-target PRIVATE utest)
```

To include gtest and userver-specific macros, do:

```cpp
#include <userver/utest/utest.hpp>
```

The header provides alternative gtest-like macros that run tests in a coroutine environment:

*  UTEST(test_suite_name, test_name)
*  UTEST_MT(test_suite_name, test_name, thread_count)
*  UTEST_F(test_suite_name, test_name)
*  UTEST_F_MT(test_suite_name, test_name, thread_count)
*  UTEST_P(test_suite_name, test_name)
*  UTEST_P_MT(test_suite_name, test_name, thread_count)
*  UTEST_DEATH(test_suite_name, test_name)
*  TYPED_UTEST(test_suite_name, test_name)
*  TYPED_UTEST_MT(test_suite_name, test_name, thread_count)
*  TYPED_UTEST_P(test_suite_name, test_name)
*  TYPED_UTEST_P_MT(test_suite_name, test_name, thread_count)
*  TYPED_UTEST_SUITE(test_suite_name, types)
*  #INSTANTIATE_UTEST_SUITE_P(prefix, test_suite_name, ...)
*  #REGISTER_TYPED_UTEST_SUITE_P(test_suite_name, ...)
*  INSTANTIATE_TYPED_UTEST_SUITE_P(prefix, test_suite_name, types)
*  TYPED_UTEST_SUITE_P(test_suite_name)

As usual, gmock is available in `<gmock/gmock.h>`.

See also utest::PrintTestName for info on how to simplify writing parametrized tests and [official gtest documentation](https://google.github.io/googletest/).

### Coroutine environment

To test code that uses coroutine environment (e.g. creates tasks or uses [synchronization primitives](/en/userver/synchronization.md)), use `UTEST` instead of `TEST` in the test header:

@snippet core/src/engine/semaphore_test.cpp  UTEST macro example 1

There are `U`-versions for all gtest macros that declare tests and test groups: `UTEST_F`, `UTEST_P`, `INSTANTIATE_UTEST_SUITE_P` etc. Test fixture's constructor, destructor, `SetUp` and `TearDown` are executed in the same coroutine environment as the test body.

By default, a single-threaded `TaskProcessor` is used. It is usually enough, because userver engine enables concurrency even on 1 thread. However, some tests require actual parallelism, e.g. torture tests that search for potential data races. In such cases, use `_MT` macro versions:

@snippet core/src/engine/semaphore_test.cpp  UTEST macro example 2

The specified thread count is available in `U`-tests as `GetThreadCount()` method.

For DEATH-tests (when testing aborts or assertion fails) use `UTEST_DEATH`. It configures gtest-DEATH-checks to work in multithreaded environment. Also it disables using of `ev_default_loop` and catching of `SIGCHLD` signal to work with gtest's `waitpid()` calls.

### Exception assertions

Standard gtest exception assertions provide poor error messages. Their equivalents with proper diagnostics are available in `<userver/utest/utest.hpp>`:

* UEXPECT_THROW_MSG(statement, exception_type, message_substring)
* UASSERT_THROW_MSG(statement, exception_type, message_substring)
* UEXPECT_THROW(statement, exception_type)
* UASSERT_THROW(statement, exception_type)
* UEXPECT_NO_THROW(statement)
* UASSERT_NO_THROW(statement)

Example usage:

@snippet core/testing/src/utest/assert_macros_test.cpp  Sample assert macros usage

@anchor utest-mocked-time
### Mocked time

- To mock time, use `utils::datetime::Now()` and `utils::datetime::SteadyNow()` from `<userver/utils/datetime.hpp>` instead of `std::chrono::system_clock::now()` and `std::chrono::steady_clock::now()`, respectively 
- Enable mocked time controls by adding `MOCK_NOW=1` preprocessor definition to your test CMake target
- Control the mocked time in tests using `<userver/utils/mock_now.hpp>`

@snippet shared/src/utils/mock_now_test.cpp  Mocked time sample

@anchor utest-dynamic-config
### Mocked dynamic config

You can fill dynamic config with custom config values
using `dynamic_config::StorageMock`.

@snippet core/src/dynamic_config/config_test.cpp Sample StorageMock usage

If you don't want to specify all configs used by the tested code, you can use default dynamic config.

To use default dynamic config values in tests, add
`DEFAULT_TAXI_CONFIG_FILENAME` preprocessor definition to your test CMake target, specifying the path of a YAML file
with `dynamic_config::DocsMap` contents.

Default dynamic config values can be accessed using `<dynamic_config/test_helpers.hpp>`:

- `dynamic_config::GetDefaultSnapshot()`
- `dynamic_config::GetDefaultSource()`
- `dynamic_config::MakeDefaultStorage(overrides)`

```cpp
// Some production code
void MyHelper(const dynamic_config::Snapshot&);

class MyClient final {
  explicit MyClient(dynamic_config::Source);
};

// Tests
TEST(Stuff, DefaultConfig) {
  MyHelper(dynamic_config::GetDefaultSnapshot());
  MyClient client{dynamic_config::GetDefaultSource()};
}

TEST(Stuff, CustomConfig) {
  const auto config_storage = dynamic_config::MakeDefaultStorage({
      {kThisConfig, this_value},
      {kThatConfig, that_value},
  });
  MyHelper(config_storage.GetSnapshot());
  MyClient client{config_storage.GetSource()};
}
```

## Benchmarks (google-benchmark)

### Getting started

All userver benchmark helpers live in `ubench` CMake target:

```cmake
target_link_libraries(your-bench-target PRIVATE ubench)
```

As usual, google-benchmark is available in `<benchmark/benchmark.h>`.

See also official google-benchmark [documentation](https://github.com/google/benchmark/blob/main/README.md).

### Coroutine environment

Use `engine::RunStandalone` to run parts of your benchmark in a coroutine environment:

@snippet core/src/engine/semaphore_benchmark.cpp  RunStandalone sample

### Mocked dynamic config

See the [equivalent utest section](#utest-dynamic-config).

Default dynamic configs are available
in `<userver/dynamic_config/benchmark_helpers.hpp>`.

## Functional service tests (testsuite)

### Getting started

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

### CMake integration

With `userver_testsuite_add()` function you can easily add testsuite support to your project.
Its main purpose is:

* Setup Python environment `virtualenv` or  uses existing one.
* Create runner script that setups `PYTHONPATH` and passes extra arguments to `pytest`.
* Registers `ctest` target.


@snippet testsuite/SetupUserverTestsuiteEnv.cmake testsuite - UserverTestsuite
@snippet samples/CMakeLists.txt testsuite - cmake

@ref cmake/UserverTestsuite.cmake "UserverTestsuite" CMake library is automatically addded to
CMake path after userever enviroment setup.

#### Arguments

* NAME, required test name used as `ctest` target name.
* WORKING_DIRECTORY, pytest working directory. Default is ${CMAKE_CURRENT_SOURCE_DIR}.
* PYTEST_ARGS, list of extra arguments passed to `pytest`.
* PYTHONPATH, list of directories to be prepended to `PYTHONPATH`.
* REQUIREMENTS, list of reqirements.txt files used to populate `virtualenv`.
* PYTHON_BINARY, path to existing Python binary.


#### Python environment

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

#### Run with ctest

`userver_testsuite_add()` registers a ctest target with name NAME.
Run all project tests with ctest command or use filters to run specific tests:

```shell
ctest -V -R my-project  # NAME argument is used
```

#### Direct run

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

### pytest_userver (WIP)

By default internal `pytest_userver` plugin is included in python path.
To use it add it to your `pytest_plugins` in root `conftest.py`:

@snippet samples/conftest.py testsuite - pytest_plugins

It requires extra PYTEST_ARGS to be passed:

@snippet samples/CMakeLists.txt testsuite - cmake

#### Fixtures

### Features
#### Mockserver

[Mockserver](https://yandex.github.io/yandex-taxi-testsuite/mockserver/) allows to mock external
HTTP handlers. It starts its own HTTP server that receives HTTP traffic from service being tested.
And allows to install custom HTTP handlers within testsuite.
In order to use it all HTTP clients must be pointed to mockserver address.

Mockserver usage example:

@snippet samples/http_caching/tests/conftest.py mockserver - json_handler

* Testcase: @ref samples/http_caching/tests/conftest.py

#### Mock time

Userver provides a way to mock internal datetime value. It only works for datetime retrieved
with `utils::datetime::Now()`, see [Mocked time section](#utest-mocked-time) for details.

From testsuite you can control it with [mocked_time](https://yandex.github.io/yandex-taxi-testsuite/other_plugins/#mocked-time)
plugin.

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

* C++ code: @ref samples/testsuite-support/testpoint.cpp
* Testcase: @ref samples/testsuite-support/tests/test_testpoint.py

#### Logs capture

Testsuite can be used to test logs written by service.
To achieve this the testsuite starts a simple logs capture TCP server and
tells the service to replicate logs to it on per test basis.

Example on logs capture usage could be found here:

* C++ code: @ref samples/testsuite-support/logcapture.cpp
* Testcase: @ref samples/testsuite-support/tests/test_logcapture.py

@example cmake/UserverTestsuite.cmake
@example samples/http_caching/tests/conftest.py
@example samples/testsuite-support/logcapture.cpp
@example samples/testsuite-support/now.cpp
@example samples/testsuite-support/testpoint.cpp
@example samples/testsuite-support/tests/test_logcapture.py
@example samples/testsuite-support/tests/test_mocked_time.py
@example samples/testsuite-support/tests/test_testpoint.py
