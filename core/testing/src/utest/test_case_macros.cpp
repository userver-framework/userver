#include <utest/test_case_macros.hpp>

#include <string>

#include <fmt/format.h>
#include <boost/stacktrace.hpp>

#include <compiler/demangle.hpp>
#include <logging/stacktrace_cache.hpp>
#include <utils/traceful_exception.hpp>

namespace utest::impl {

namespace {

std::string TraceToStringIfAny(const std::exception& ex) {
  const auto* traceful = dynamic_cast<const utils::TracefulExceptionBase*>(&ex);
  if (!traceful) return {};
  return logging::stacktrace_cache::to_string(traceful->Trace());
}

}  // namespace

void LogFatalException(const std::exception& ex, const char* name) {
  const auto trace = TraceToStringIfAny(ex);
  const auto newline = trace.empty() ? "" : "\n";

  const auto message = fmt::format(
      "C++ exception with description \"{}\" ({}) thrown in {}.{}{}", ex.what(),
      compiler::GetTypeName(typeid(ex)), name, newline, trace);
  GTEST_FATAL_FAILURE_(message.c_str());
}

void LogUnknownFatalException(const char* name) {
  const auto message = fmt::format("Unknown C++ exception thrown in {}.", name);
  GTEST_FATAL_FAILURE_(message.c_str());
}

}  // namespace utest::impl
