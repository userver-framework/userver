#include <userver/logging/log_extra.hpp>

#include <gtest/gtest.h>

#include <userver/logging/log.hpp>
#include <userver/utest/assert_macros.hpp>

// TODO move these tests to `shared`. Can't move right now, because
//  there is no `EXPECT_UINVARIANT_FAILURE` in `shared`

USERVER_NAMESPACE_BEGIN

TEST(LogExtra, Types) {
  logging::LogExtra le{
      {"unsigned int", 1U},          //
      {"int", 1},                    //
      {"long", 1L},                  //
      {"long long", 1LL},            //
      {"unsigned long", 1UL},        //
      {"unsigned long long", 1ULL},  //
      {"size_t", size_t{1}},         //
  };
}

TEST(LogExtra, DocsData) {
  /// [Example using LogExtra]
  logging::LogExtra log_extra;
  log_extra.Extend("key", "value");
  LOG_INFO() << log_extra << "message";
  /// [Example using LogExtra]

  /// [Example using stacktrace in log]
  LOG_ERROR() << "Deadlock in ABC identified"
              << logging::LogExtra::Stacktrace();
  /// [Example using stacktrace in log]

  // Check that logging LogExtra::Stacktrace and LogFlush don't require a
  // coroutine environment.
  logging::LogFlush();
}

TEST(LogExtraDeathTest, UsingTechnicalKeys) {
  testing::FLAGS_gtest_death_test_style = "threadsafe";

  logging::LogExtra log_extra;
  for (const auto& key :
       {"timestamp", "level", "module", "task_id", "thread_id", "text"}) {
    EXPECT_UINVARIANT_FAILURE(log_extra.Extend(key, 1));
  }
}

USERVER_NAMESPACE_END
