#include <userver/utest/utest.hpp>

#include <gmock/gmock.h>

#include <logging/dynamic_debug.hpp>
#include <logging/logging_test.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingTest, DynamicDebugEnable) {
  const std::string filename{USERVER_FILEPATH};
  SetDefaultLoggerLevel(logging::Level::kNone);

  const auto do_log = [](std::string_view string) {
#line 10001
    LOG_INFO() << string;
  };

  do_log("before");
  LOG_INFO() << "unrelated 1";

  logging::AddDynamicDebugLog(filename, 10001);

  do_log("enabled");
  LOG_INFO() << "unrelated 2";

  logging::RemoveDynamicDebugLog(filename, 10001);

  do_log("after");
  LOG_INFO() << "unrelated 3";

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("enabled"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("unrelated")));
}

TEST_F(LoggingTest, DynamicDebugDisable) {
  const std::string filename{USERVER_FILEPATH};
  SetDefaultLoggerLevel(logging::Level::kInfo);

  const auto do_log = [](std::string_view string) {
#line 20001
    LOG_INFO() << string;
  };

  logging::EntryState state;
  state.force_disabled_level_plus_one = logging::Level::kWarning;
  logging::AddDynamicDebugLog(filename, 20001, state);

  do_log("here");

  logging::RemoveDynamicDebugLog(filename, 20001);

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("here")));
}

TEST_F(LoggingTest, DynamicDebugAnyLine) {
  const std::string filename{USERVER_FILEPATH};
  SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  logging::AddDynamicDebugLog(filename, logging::kAnyLine);

  LOG_INFO() << "123";
  LOG_INFO() << "456";

  logging::RemoveDynamicDebugLog(filename, logging::kAnyLine);

  LOG_INFO() << "after";

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("123"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("456"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
}

TEST_F(LoggingTest, DynamicDebugAnyLineRemove) {
  SetDefaultLoggerLevel(logging::Level::kNone);

  LOG_INFO() << "before";

  const std::string location{USERVER_FILEPATH};
  logging::AddDynamicDebugLog(location, 30001);
  logging::AddDynamicDebugLog(location, 30002);
  logging::RemoveDynamicDebugLog(location, logging::kAnyLine);

#line 30001
  LOG_INFO() << "123";
  LOG_INFO() << "456";

  const std::string kBadPath = "Non existing path (*&#(R&!(!@(*)*#&)@#$!";
  UEXPECT_THROW_MSG(logging::AddDynamicDebugLog(kBadPath, 1),
                    std::runtime_error, kBadPath);

  const int kBadLine{98888988};
  UEXPECT_THROW_MSG(logging::AddDynamicDebugLog(location, kBadLine),
                    std::runtime_error, "98888988");

  EXPECT_FALSE(LoggedTextContains("before"));
  EXPECT_FALSE(LoggedTextContains("123"));
  EXPECT_FALSE(LoggedTextContains("456"));
  EXPECT_FALSE(LoggedTextContains("after"));
}

TEST_F(LoggingTest, DynamicDebugLimitedEnable) {
  const std::string location{USERVER_FILEPATH};
  SetDefaultLoggerLevel(logging::Level::kNone);

  const auto do_log = [](std::string_view string) {
#line 40001
    LOG_LIMITED_INFO() << string;
  };

  do_log("before");
  LOG_LIMITED_INFO() << "unrelated 1";

  logging::AddDynamicDebugLog(location, 40001);

  do_log("enabled 1");
  do_log("enabled 2");
  do_log("enabled 3");
  do_log("enabled 4");
  do_log("enabled 5");
  LOG_LIMITED_INFO() << "unrelated 2";

  logging::RemoveDynamicDebugLog(location, 40001);

  do_log("after");
  LOG_LIMITED_INFO() << "unrelated 3";

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
  // TODO(TAXICOMMON-7307) log attempts, which were earlier discarded by level,
  //  affect rate limit of new log attempts (and they probably shouldn't)
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("enabled 1"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("enabled 2")));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("enabled 3"));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("enabled 4")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("enabled 5")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("unrelated")));
}

TEST_F(LoggingTest, DynamicDebugEnableLevel) {
  const std::string filename{USERVER_FILEPATH};
  SetDefaultLoggerLevel(logging::Level::kNone);

  const auto do_log_trace = [](std::string_view string) {
#line 50001
    LOG_TRACE() << string;
  };

  const auto do_log_info = [](std::string_view string) {
#line 50002
    LOG_INFO() << string;
  };

  const auto do_log_error = [](std::string_view string) {
#line 50003
    LOG_ERROR() << string;
  };

  do_log_error("before");
  LOG_INFO() << "unrelated 1";

  logging::EntryState state1;
  state1.force_enabled_level = logging::Level::kError;
  logging::AddDynamicDebugLog(filename, 50001, state1);
  logging::AddDynamicDebugLog(filename, 50002, state1);
  logging::AddDynamicDebugLog(filename, 50003, state1);

  do_log_trace("trace enabled 1");
  do_log_info("info enabled 1");
  do_log_error("error enabled 1");
  LOG_INFO() << "unrelated 2";

  logging::EntryState state2;
  state2.force_enabled_level = logging::Level::kInfo;
  logging::AddDynamicDebugLog(filename, 50001, state2);
  logging::AddDynamicDebugLog(filename, 50002, state2);
  logging::AddDynamicDebugLog(filename, 50003, state2);

  do_log_trace("trace enabled 2");
  do_log_info("info enabled 2");
  do_log_error("error enabled 2");
  LOG_INFO() << "unrelated 3";

  logging::EntryState state3;
  state3.force_enabled_level = logging::Level::kTrace;
  logging::AddDynamicDebugLog(filename, 50001, state3);
  logging::AddDynamicDebugLog(filename, 50002, state3);
  logging::AddDynamicDebugLog(filename, 50003, state3);

  do_log_trace("trace enabled 3");
  do_log_info("info enabled 3");
  do_log_error("error enabled 3");
  LOG_INFO() << "unrelated 4";

  logging::RemoveDynamicDebugLog(filename, 50001);
  logging::RemoveDynamicDebugLog(filename, 50002);
  logging::RemoveDynamicDebugLog(filename, 50003);

  do_log_trace("trace after");
  do_log_info("info after");
  do_log_error("error after");
  LOG_INFO() << "unrelated 5";

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("error enabled 1"));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("info enabled 1")));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("trace enabled 1")));

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("error enabled 2"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("info enabled 2"));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("trace enabled 2")));

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("error enabled 3"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("info enabled 3"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("trace enabled 3"));

  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
  EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("unrelated")));
}

TEST_F(LoggingTest, DynamicDebugDisableLevel) {
  const std::string filename{USERVER_FILEPATH};
  SetDefaultLoggerLevel(logging::Level::kInfo);

  const auto do_log_trace = [](std::string_view string) {
#line 60001
    LOG_TRACE() << string;
  };

  const auto do_log_info = [](std::string_view string) {
#line 60002
    LOG_INFO() << string;
  };

  const auto do_log_error = [](std::string_view string) {
#line 60003
    LOG_ERROR() << string;
  };

  logging::EntryState state1;
  state1.force_disabled_level_plus_one = logging::Level::kDebug;
  logging::AddDynamicDebugLog(filename, 60001, state1);
  logging::AddDynamicDebugLog(filename, 60002, state1);
  logging::AddDynamicDebugLog(filename, 60003, state1);

  do_log_trace("trace disabled 1");
  do_log_info("info disabled 1");
  do_log_error("error disabled 1");

  logging::EntryState state2;
  state2.force_disabled_level_plus_one = logging::Level::kWarning;
  logging::AddDynamicDebugLog(filename, 60001, state2);
  logging::AddDynamicDebugLog(filename, 60002, state2);
  logging::AddDynamicDebugLog(filename, 60003, state2);

  do_log_trace("trace disabled 2");
  do_log_info("info disabled 2");
  do_log_error("error disabled 2");

  logging::EntryState state3;
  state3.force_disabled_level_plus_one = logging::Level::kCritical;
  logging::AddDynamicDebugLog(filename, 60001, state3);
  logging::AddDynamicDebugLog(filename, 60002, state3);
  logging::AddDynamicDebugLog(filename, 60003, state3);

  do_log_trace("trace disabled 3");
  do_log_info("info disabled 3");
  do_log_error("error disabled 3");

  logging::RemoveDynamicDebugLog(filename, 60001);
  logging::RemoveDynamicDebugLog(filename, 60002);
  logging::RemoveDynamicDebugLog(filename, 60003);

  LOG_INFO() << "unrelated";

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("error disabled 1"));
  EXPECT_THAT(GetStreamString(), testing::HasSubstr("info disabled 1"));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("trace disabled 1")));

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("error disabled 2"));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("info disabled 2")));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("trace disabled 2")));

  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("error disabled 3")));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("info disabled 3")));
  EXPECT_THAT(GetStreamString(),
              testing::Not(testing::HasSubstr("trace disabled 3")));

  EXPECT_THAT(GetStreamString(), testing::HasSubstr("unrelated"));
}

USERVER_NAMESPACE_END
