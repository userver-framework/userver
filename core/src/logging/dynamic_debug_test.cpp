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

    logging::AddDynamicDebugLog(filename.substr(0, filename.size() - 4), logging::kAnyLine);
    do_log("by_prefix");
    logging::RemoveDynamicDebugLog(filename.substr(0, filename.size() - 4), logging::kAnyLine);
    do_log("after by_prefix");

    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("enabled"));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("unrelated")));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("by_prefix"));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after by_prefix")));

    EXPECT_THROW(logging::AddDynamicDebugLog("i/do/not/exist.cpp", 999), std::exception);
    EXPECT_THROW(logging::AddDynamicDebugLog("i/do/not/exist.cpp", 0), std::exception);
    EXPECT_THROW(logging::AddDynamicDebugLog("i/do/not/exist", 0), std::exception);

    EXPECT_THROW(logging::RemoveDynamicDebugLog("i/do/not/exist.cpp", 999), std::exception);
    EXPECT_THROW(logging::RemoveDynamicDebugLog("i/do/not/exist.cpp", 0), std::exception);
    EXPECT_THROW(logging::RemoveDynamicDebugLog("i/do/not/exist", 0), std::exception);
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
    logging::SetDynamicDebugLog(filename, 20001, state);

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

    const std::string bad_path = "Non existing path (*&#(R&!(!@(*)*#&)@#$!";
    UEXPECT_THROW_MSG(logging::AddDynamicDebugLog(bad_path, 1), std::runtime_error, bad_path);

    const int bad_line{98888988};
    UEXPECT_THROW_MSG(logging::AddDynamicDebugLog(location, bad_line), std::runtime_error, "98888988");

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
    logging::SetDynamicDebugLog(filename, 50001, state1);
    logging::SetDynamicDebugLog(filename, 50002, state1);
    logging::SetDynamicDebugLog(filename, 50003, state1);

    do_log_trace("trace enabled 1");
    do_log_info("info enabled 1");
    do_log_error("error enabled 1");
    LOG_INFO() << "unrelated 2";

    logging::EntryState state2;
    state2.force_enabled_level = logging::Level::kInfo;
    logging::SetDynamicDebugLog(filename, 50001, state2);
    logging::SetDynamicDebugLog(filename, 50002, state2);
    logging::SetDynamicDebugLog(filename, 50003, state2);

    do_log_trace("trace enabled 2");
    do_log_info("info enabled 2");
    do_log_error("error enabled 2");
    LOG_INFO() << "unrelated 3";

    logging::EntryState state3;
    state3.force_enabled_level = logging::Level::kTrace;
    logging::SetDynamicDebugLog(filename, 50001, state3);
    logging::SetDynamicDebugLog(filename, 50002, state3);
    logging::SetDynamicDebugLog(filename, 50003, state3);

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
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("info enabled 1")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("trace enabled 1")));

    EXPECT_THAT(GetStreamString(), testing::HasSubstr("error enabled 2"));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("info enabled 2"));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("trace enabled 2")));

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
    logging::SetDynamicDebugLog(filename, 60001, state1);
    logging::SetDynamicDebugLog(filename, 60002, state1);
    logging::SetDynamicDebugLog(filename, 60003, state1);

    do_log_trace("trace disabled 1");
    do_log_info("info disabled 1");
    do_log_error("error disabled 1");

    logging::EntryState state2;
    state2.force_disabled_level_plus_one = logging::Level::kWarning;
    logging::SetDynamicDebugLog(filename, 60001, state2);
    logging::SetDynamicDebugLog(filename, 60002, state2);
    logging::SetDynamicDebugLog(filename, 60003, state2);

    do_log_trace("trace disabled 2");
    do_log_info("info disabled 2");
    do_log_error("error disabled 2");

    logging::EntryState state3;
    state3.force_disabled_level_plus_one = logging::Level::kCritical;
    logging::SetDynamicDebugLog(filename, 60001, state3);
    logging::SetDynamicDebugLog(filename, 60002, state3);
    logging::SetDynamicDebugLog(filename, 60003, state3);

    do_log_trace("trace disabled 3");
    do_log_info("info disabled 3");
    do_log_error("error disabled 3");

    logging::RemoveDynamicDebugLog(filename, 60001);
    logging::RemoveDynamicDebugLog(filename, 60002);
    logging::RemoveDynamicDebugLog(filename, 60003);

    LOG_INFO() << "unrelated";

    EXPECT_THAT(GetStreamString(), testing::HasSubstr("error disabled 1"));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("info disabled 1"));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("trace disabled 1")));

    EXPECT_THAT(GetStreamString(), testing::HasSubstr("error disabled 2"));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("info disabled 2")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("trace disabled 2")));

    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("error disabled 3")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("info disabled 3")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("trace disabled 3")));

    EXPECT_THAT(GetStreamString(), testing::HasSubstr("unrelated"));
}

TEST_F(LoggingTest, DynamicDebugEnableSameLine) {
    const std::string filename{USERVER_FILEPATH};
    SetDefaultLoggerLevel(logging::Level::kNone);

    const auto do_log = [](std::string_view string) {
    // clang-format off
#line 70001
        LOG_INFO() << "@1@: " << string; LOG_INFO() << "@2@: " << string; LOG_INFO() << "@3@: " << string;
        // clang-format on
    };

    do_log("before");
    LOG_INFO() << "unrelated 1";

    logging::AddDynamicDebugLog(filename, 70001);

    do_log("enabled");
    LOG_INFO() << "unrelated 2";

    logging::RemoveDynamicDebugLog(filename, 70001);

    do_log("after");
    LOG_INFO() << "unrelated 3";

    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("@1@: enabled"));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("@2@: enabled"));
    EXPECT_THAT(GetStreamString(), testing::HasSubstr("@3@: enabled"));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("unrelated")));
}

TEST_F(LoggingTest, DynamicDebugEnableMultilineMacro) {
    const std::string filename{USERVER_FILEPATH};
    SetDefaultLoggerLevel(logging::Level::kNone);

    const auto do_log = [](std::string_view string_value_that_must_be_long_for_this_test) {
#line 80001
        LOG_INFO(
            "Value 1 is '{}', making this line very long to make the log macro consume multiple lines in sources. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );

#line 80101
        LOG_LIMITED_INFO(
            "Value 2 is '{}', making this line very long to make the log macro consume multiple lines in sources. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );

#line 80201
        LOG(logging::Level::kInfo,
            "Value 3 is '{}', making this line very long to make the log macro consume multiple lines. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test);

#line 80301
        LOG_INFO_TO(
            logging::GetDefaultLogger(),
            "Value 4 is '{}', making this line very long to make the log macro consume multiple lines. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );

#line 80401
        LOG_LIMITED_INFO_TO(
            logging::GetDefaultLogger(),
            "Value 5 is '{}', making this line very long to make the log macro consume multiple lines. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );

#line 80501
        LOG_TO(
            logging::GetDefaultLogger(),
            logging::Level::kInfo,
            "Value 6 is '{}', making this line very long to make the log macro consume multiple lines. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );

#line 80601
        LOG_LIMITED_TO(
            logging::GetDefaultLogger(),
            logging::Level::kInfo,
            "Value 7 is '{}', making this line very long to make the log macro consume multiple lines. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );

#line 80701
        LOG_LIMITED(
            logging::Level::kInfo,
            "Value 8 is '{}', making this line very long to make the log macro consume multiple lines. {}",
            string_value_that_must_be_long_for_this_test,
            string_value_that_must_be_long_for_this_test
        );
    };

    do_log("before");
    LOG_INFO() << "unrelated 1";

    // The preprocessor behavior is platform-dependent
    constexpr int kMaxLinesLengthOfMacro = 7;
    for (int line = 80001; line < 80800; line += 100) {
        bool success = false;
        for (int i = 0; i < kMaxLinesLengthOfMacro; ++i) {
            try {
                logging::AddDynamicDebugLog(filename, line + i);
                success = true;
                break;
            } catch (const std::exception&) {
            }
        }
        ASSERT_TRUE(success) << "Failed log enabling for line: " << line;
    }

    do_log("enabled");
    LOG_INFO() << "unrelated 2";

    for (int line = 80001; line < 80800; line += 100) {
        bool success = false;
        for (int i = 0; i < kMaxLinesLengthOfMacro; ++i) {
            try {
                logging::RemoveDynamicDebugLog(filename, line + i);
                success = true;
                break;
            } catch (const std::exception&) {
            }
        }
        ASSERT_TRUE(success) << "Failed log disabling for line: " << line;
    }

    do_log("after");
    LOG_INFO() << "unrelated 3";

    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("before")));
    for (std::size_t i = 1; i < 9; ++i) {
        EXPECT_THAT(GetStreamString(), testing::HasSubstr(fmt::format("Value {} is 'enabled'", i)));
    }
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("after")));
    EXPECT_THAT(GetStreamString(), testing::Not(testing::HasSubstr("unrelated")));
}

USERVER_NAMESPACE_END
