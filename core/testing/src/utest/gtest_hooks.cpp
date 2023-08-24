#include <utest/gtest_hooks.hpp>

#include <gtest/gtest.h>

#include <engine/task/exception_hacks.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/impl/static_registration.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
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

void InitPhdrCache() {
  static USERVER_NAMESPACE::utils::impl::UserverExperimentsScope
      phdr_cache_scope{};
  phdr_cache_scope.Set(USERVER_NAMESPACE::utils::impl::kPhdrCacheExperiment,
                       true);

  USERVER_NAMESPACE::engine::impl::InitPhdrCacheAndDisableDynamicLoading(
      USERVER_NAMESPACE::engine::impl::DebugInfoAction::kLockInMemory);
}

void TeardownPhdrCache() {
  USERVER_NAMESPACE::engine::impl::TeardownPhdrCacheAndEnableDynamicLoading();
}

}  // namespace utest::impl

USERVER_NAMESPACE_END
