#include <userver/utest/test_case_macros.hpp>

#include <string>

#include <fmt/format.h>
#include <boost/stacktrace.hpp>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

namespace {

std::string TraceToStringIfAny(const std::exception& ex) {
  const auto* traceful = dynamic_cast<const utils::TracefulExceptionBase*>(&ex);
  if (!traceful) return {};
  return logging::stacktrace_cache::to_string(traceful->Trace());
}

void LogFatalException(const std::exception& ex, const char* name) {
  const auto trace = TraceToStringIfAny(ex);
  const auto newline = trace.empty() ? "" : "\n";

  const auto message = fmt::format(
      "C++ exception with description \"{}\" ({}) thrown in {}.{}{}", ex.what(),
      compiler::GetTypeName(typeid(ex)), name, newline, trace);
  GTEST_FAIL_AT("unknown file", -1) << message;
}

void LogUnknownFatalException(const char* name) {
  const auto message = fmt::format("Unknown C++ exception thrown in {}.", name);
  GTEST_FAIL_AT("unknown file", -1) << message;
}

template <typename Func>
decltype(auto) CallLoggingExceptions(const char* name, const Func& func) {
  try {
    return func();
  } catch (const std::exception& ex) {
    LogFatalException(ex, name);
    return decltype(func())();
  } catch (...) {
    LogUnknownFatalException(name);
    return decltype(func())();
  }
}

void DoRunTest(std::size_t worker_threads,
               const engine::TaskProcessorPoolsConfig& config,
               std::function<std::unique_ptr<EnrichedTestBase>()> factory) {
  engine::RunStandalone(worker_threads, config, [&] {
    auto test =
        CallLoggingExceptions("the test fixture's constructor", factory);
    if (test->IsTestCancelled()) return;

    test->SetThreadCount(worker_threads);

    utils::ScopeGuard tear_down_guard{[&] {
      // gtest invokes TearDown even if SetUp fails
      CallLoggingExceptions("TearDown()", [&] { test->TearDown(); });
    }};

    CallLoggingExceptions("SetUp()", [&] { test->SetUp(); });
    if (test->IsTestCancelled()) return;

    CallLoggingExceptions("the test body", [&] { test->TestBody(); });
  });
}

}  // namespace

void DoRunTest(std::size_t thread_count,
               std::function<std::unique_ptr<EnrichedTestBase>()> factory) {
  return DoRunTest(thread_count, {}, std::move(factory));
}

void DoRunDeathTest(
    std::size_t thread_count,
    std::function<std::unique_ptr<EnrichedTestBase>()> factory) {
  engine::TaskProcessorPoolsConfig config{};
  // Disable using of `ev_default_loop` and catching of `SIGCHLD` signal to work
  // with gtest's `waitpid()` calls.
  config.ev_default_loop_disabled = true;
  return DoRunTest(thread_count, config, std::move(factory));
}

}  // namespace utest::impl

USERVER_NAMESPACE_END
