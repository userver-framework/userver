# Unit Tests and Benchmarks

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

* #UEXPECT_THROW_MSG(statement, exception_type, message_substring)
* #UASSERT_THROW_MSG(statement, exception_type, message_substring)
* #UEXPECT_THROW(statement, exception_type)
* #UASSERT_THROW(statement, exception_type)
* #UEXPECT_NO_THROW(statement)
* #UASSERT_NO_THROW(statement)

Example usage:

@snippet core/testing/src/utest/assert_macros_test.cpp  Sample assert macros usage

@anchor utest-mocked-time
### Mocked time

- To mock time, use `utils::datetime::Now()` and `utils::datetime::SteadyNow()` from `<userver/utils/datetime.hpp>` instead of `std::chrono::system_clock::now()` and `std::chrono::steady_clock::now()`, respectively 
- Control the mocked time in tests using `<userver/utils/mock_now.hpp>`

@snippet shared/src/utils/mock_now_test.cpp  Mocked time sample

@anchor utest-dynamic-config
### Mocked dynamic config

You can fill dynamic config with custom config values
using `dynamic_config::StorageMock`.

@snippet core/src/dynamic_config/config_test.cpp Sample StorageMock usage

If you don't want to specify all configs used by the tested code, you can use default dynamic config.

To use default dynamic config values in tests, add
`DEFAULT_DYNAMIC_CONFIG_FILENAME` preprocessor definition to your test CMake target, specifying the path of a YAML file
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


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref md_en_userver_periodics | @ref md_en_userver_functional_testing ⇨
@htmlonly </div> @endhtmlonly
