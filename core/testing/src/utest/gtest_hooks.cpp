#include <utest/gtest_hooks.hpp>

#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/impl/static_registration.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

namespace {

class ResetMockNowListener : public ::testing::EmptyTestEventListener {
  void OnTestEnd(const ::testing::TestInfo&) override {
    utils::datetime::MockNowUnset();
  }
};

}  // namespace

void FinishStaticInit() {
  USERVER_NAMESPACE::utils::impl::FinishStaticRegistration();
}

void InitMockNow() {
  auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new ResetMockNowListener());
}

void SetLogLevel(logging::Level log_level) {
  if (log_level <= logging::Level::kTrace) {
    // A hack for avoiding Boost.Stacktrace memory leak
    logging::stacktrace_cache::GlobalEnableStacktrace(false);
  }

  static logging::DefaultLoggerGuard logger{
      logging::MakeStderrLogger("default", logging::Format::kTskv, log_level)};

  logging::SetDefaultLoggerLevel(log_level);
}

}  // namespace utest::impl

USERVER_NAMESPACE_END
