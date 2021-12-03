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

See also utest::PrintTestName for info on how to simplify writing paramtrized tests and [official gtest documentation](https://google.github.io/googletest/).

### Coroutine environment

To test code that uses coroutine environment (e.g. creates tasks or uses [synchronization primitives](/en/userver/synchronization.md)), use `UTEST` instead of `TEST` in the test header:

@snippet core/src/engine/semaphore_test.cpp  UTEST macro example 1

There are `U`-versions for all gtest macros that declare tests and test groups: `UTEST_F`, `UTEST_P`, `INSTANTIATE_UTEST_SUITE_P` etc. Test fixture's constructor, destructor, `SetUp` and `TearDown` are executed in the same coroutine environment as the test body.

By default, a single-threaded `TaskProcessor` is used. It is usually enough, because userver engine enables concurrency even on 1 thread. However, some tests require actual parallelism, e.g. torture tests that search for potential data races. In such cases, use `_MT` macro versions:

@snippet core/src/engine/semaphore_test.cpp  UTEST macro example 2

The specified thread count is available in `U`-tests as `GetThreadCount()` method.

For DEATH-tests (when testing aborts or assertion fails) use `UTEST_DEATH`. It configures gtest-DEATH-checks to work in multithreaded environment. Also it disables using of `ev_default_loop` and catching of `SIGCHLD` signal to work with gtest's `waitpid()` calls.

### Mocked time

- To mock time, use `utils::datetime::Now()` and `utils::datetime::SteadyNow()` from `<userver/utils/datetime.hpp>` instead of `std::chrono::system_clock::now()` and `std::chrono::steady_clock::now()`, respectively 
- Enable mocked time controls by adding `MOCK_NOW=1` preprocessor definition to your test CMake target
- Control the mocked time in tests using `<userver/utils/mock_now.hpp>`

@snippet shared/src/utils/mock_now_test.cpp  Mocked time sample

### Mocked taxi config @anchor utest-taxi-config

You can fill taxi config with custom config values using `taxi_config::StorageMock`.

@snippet core/src/taxi_config/config_test.cpp  Sample StorageMock usage

If you don't want to specify all configs used by the tested code, you can use default taxi config.

To use default taxi config values in tests, add `DEFAULT_TAXI_CONFIG_FILENAME` preprocessor definition to your test CMake target, specifying the path of a YAML file with `taxi_config::DocsMap` contents.

Default taxi config values can be accessed using `<taxi_config/test_helpers.hpp>`:

- `taxi_config::GetDefaultSnapshot()`
- `taxi_config::GetDefaultSource()`
- `taxi_config::MakeDefaultStorage(overrides)`

```cpp
// Some production code
void MyHelper(const taxi_config::Snapshot&);

class MyClient final {
  explicit MyClient(taxi_config::Source);
};

// Tests
TEST(Stuff, DefaultConfig) {
  MyHelper(taxi_config::GetDefaultSnapshot());
  MyClient client{taxi_config::GetDefaultSource()};
}

TEST(Stuff, CustomConfig) {
  const auto config_storage = taxi_config::MakeDefaultStorage({
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

### Mocked taxi config

See the [equivalent utest section](#utest-taxi-config).

Default taxi configs are available in `<userver/taxi_config/benchmark_helpers.hpp>`.
