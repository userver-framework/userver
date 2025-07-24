#include <gtest/gtest.h>

#include <optional>

#include <logging/socket_logging_test.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_helper_extras.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/datetime/date.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingTest, SwitchToTraceWorks) {
    LOG_TRACE() << "test";
    SetDefaultLoggerLevel(logging::Level::kTrace);
    LOG_TRACE() << "test";
    SetDefaultLoggerLevel(logging::Level::kInfo);
    LOG_TRACE() << "test";
    SetDefaultLoggerLevel(logging::Level::kTrace);
    LOG_TRACE() << "test";
    SetDefaultLoggerLevel(logging::Level::kInfo);

    logging::LogFlush();
    const auto log_contents = GetStreamString();
    size_t pos = 0;
    size_t entries = 0;
    while ((pos = log_contents.find("text=test", pos)) != std::string::npos) {
        ++entries;
        ++pos;
    }
    EXPECT_EQ(2, entries);
}

TEST_F(LoggingTest, LogExtraExtendType) {
    SetDefaultLoggerLevel(logging::Level::kTrace);

    logging::LogExtra log_extra;
    log_extra.Extend("key1", "value1", logging::LogExtra::ExtendType::kNormal);
    LOG_TRACE() << log_extra;
    log_extra.Extend("key1", "value2", logging::LogExtra::ExtendType::kFrozen);
    LOG_TRACE() << log_extra;
    log_extra.Extend("key1", "value3", logging::LogExtra::ExtendType::kFrozen);
    LOG_TRACE() << log_extra;

    logging::LogFlush();
    const auto log_contents = GetStreamString();
    EXPECT_NE(log_contents.find("key1=value1"), std::string::npos);
    EXPECT_NE(log_contents.find("key1=value2"), std::string::npos);
    EXPECT_EQ(log_contents.find("key1=value3"), std::string::npos);
}

TEST_F(LoggingTest, MultipleFlushes) {
    SetDefaultLoggerLevel(logging::Level::kTrace);

    // Make sure that multiple flush sequences is OK
    LOG_TRACE() << "some message1";
    for (unsigned i = 0; i < 100; ++i) {
        logging::LogFlush();
    }
    LOG_TRACE() << "some message2";

    const auto log_contents = GetStreamString();
    EXPECT_NE(log_contents.find("text=some message1"), std::string::npos);
    EXPECT_NE(log_contents.find("text=some message2"), std::string::npos);
}

TEST_F(LoggingTest, ChronoDuration) {
    using namespace std::literals::chrono_literals;

    EXPECT_EQ("7ns", ToStringViaLogging(std::chrono::nanoseconds{7}));
    EXPECT_EQ("7us", ToStringViaLogging(std::chrono::microseconds{7}));
    EXPECT_EQ("7ms", ToStringViaLogging(std::chrono::milliseconds{7}));
    EXPECT_EQ("-7s", ToStringViaLogging(std::chrono::seconds{-7}));
    EXPECT_EQ("7min", ToStringViaLogging(std::chrono::minutes{7}));
    EXPECT_EQ("7h", ToStringViaLogging(std::chrono::hours{7}));
}

TEST_F(LoggingTest, Boolean) {
    EXPECT_EQ("false", ToStringViaLogging(false));
    EXPECT_EQ("true", ToStringViaLogging(true));
}

TEST_F(LoggingTest, DocsData) {
    /// [Sample logging usage]
    LOG_TRACE() << "Very verbose logs, only enabled using dynamic debug logs";
    LOG_DEBUG() << "Some debug info, not logged by default in production";
    LOG_INFO() << "This is informational message";
    LOG_WARNING() << "Something strange happened";
    LOG_ERROR() << "This is unbelievable, fix me, please!";
    LOG_CRITICAL() << "The service is about to abort, bye";
    /// [Sample logging usage]

    const bool flag = true;
    /// [Example set custom logging usage]
    const auto level = flag ? logging::Level::kDebug : logging::Level::kInfo;
    LOG(level) << "some text";
    /// [Example set custom logging usage]
}

TEST_F(LoggingTest, DocsDataInline) {
    LOG_TRACE("Very verbose logs, only enabled using dynamic debug logs");
    LOG_DEBUG("Some debug info, not logged by default in production");
    LOG_INFO("This is informational message");
    LOG_WARNING("Something strange happened");
    LOG_ERROR("This is unbelievable, fix me, please!");
    LOG_CRITICAL("The service is about to abort, bye");
}

TEST_F(LoggingTest, DocsDataInlineFormat) {
    LOG_TRACE("Very {} logs, only enabled using dynamic debug logs", "verbose");
    LOG_DEBUG("Some {} info, not logged by default in production", "debug");
    LOG_INFO("This is {} message", "informational");
    LOG_WARNING("Something {} happened", "strange");
    LOG_ERROR("This is {}, fix me, please!", "unbelievable");
    LOG_CRITICAL("The service is about to {}, bye", "abort");

    // NOTE: Should not be compilable!
    // LOG_WARNING(fmt::format("Something {} happened", "strange"));
}

TEST_F(LoggingTest, DatetimeDate) {
    const auto date = utils::datetime::Date(2023, 4, 8);
    EXPECT_EQ("2023-04-08", ToStringViaLogging(date));
    EXPECT_EQ("(none)", ToStringViaLogging(std::optional<utils::datetime::Date>{}));
    EXPECT_EQ("2023-04-08", ToStringViaLogging(std::optional<utils::datetime::Date>{date}));
}

TEST_F(LoggingTest, StdTuple) {
    const std::tuple<int> one{1234};
    EXPECT_EQ("(1234)", ToStringViaLogging(one));

    const std::tuple<int, float, char, std::string> many{42, 3.14, 'c', "str"};
    EXPECT_EQ("(42, 3.14, c, str)", ToStringViaLogging(many));

    const std::tuple<std::vector<int>> container{{1, 2, 3, 4}};
    EXPECT_EQ("([1, 2, 3, 4])", ToStringViaLogging(container));

    const std::tuple<std::optional<int>, std::optional<int>> optionals{std::nullopt, 0};
    EXPECT_EQ("((none), 0)", ToStringViaLogging(optionals));

    const std::tuple<> empty{};
    EXPECT_EQ("()", ToStringViaLogging(empty));
}

UTEST_F(SocketLoggingTest, Test) {
    LOG_ERROR() << "test";
    EXPECT_EQ("test", NextLoggedText());
}

TEST_F(LoggingTest, LogRaw) {
    logging::impl::LogRaw(*GetStreamLogger(), logging::Level::kInfo, "foo");
    EXPECT_EQ(GetStreamString(), "foo\n");
}

TEST_F(LoggingTest, DISABLED_Format) {
    LOG_ERROR("Hello, {}", "world");
    EXPECT_EQ("Hello, world", LoggedText());
    ClearLog();

    LOG_ERROR("{}, {}", "Hello", "world");
    EXPECT_EQ("Hello, world", LoggedText());
    ClearLog();
}

TEST_F(LoggingTest, Call) {
    /// [Example lambda-based logging usage]
    std::map<int, int> map{{1, 2}, {2, 3}, {3, 4}};
    LOG_ERROR() << [&map](auto& out) {
        for (const auto& [key, value] : map) {
            out << key << "=" << value << ", ";
        }
    };
    /// [Example lambda-based logging usage]
    EXPECT_EQ("1=2, 2=3, 3=4, ", LoggedText());
}

TEST_F(LoggingTest, DISABLED_CallFormat) {
    const int user_id = 42;
    const std::string ip_address = "127.0.0.1";
    const std::string_view expected_result = "User 42 logged in from 127.0.0.1";

    /// [Example format bad logging usage]
    LOG_INFO() << fmt::format("User {} logged in from {}", user_id, ip_address);
    /// [Example format bad logging usage]
    EXPECT_EQ(expected_result, LoggedText());
    ClearLog();

    /// [Example format-based logging usage]
    LOG_INFO("User {} logged in from {}", user_id, ip_address);
    /// [Example format-based logging usage]
    EXPECT_EQ(expected_result, LoggedText());
}

TEST_F(LoggingTest, LoggingContainerElementFields) {
    struct Item {
        int foo;
        std::string bar;
    };
    std::vector<Item> list{{1, "x"}, {2, "y"}};
    /// [Example Logging Container Element Fields]
    LOG_INFO() << [&list](auto& out) {
        for (const auto& item : list) {
            out << item.foo << " " << item.bar << ", ";
        }
    };
    /// [Example Logging Container Element Fields]
    EXPECT_EQ("1 x, 2 y, ", LoggedText());
}

TEST_F(LoggingTest, AddingCustomFormatting) {
    struct Item {
        int foo;
        std::string bar;
    };
    std::vector<Item> list{{1, "x"}, {2, "y"}};
    /// [Example Adding Custom Formatting]
    LOG_INFO() << [&list](auto& out) {
        bool first = true;
        out << "Items: ";
        for (const auto& item : list) {
            if (!first) out << "; ";
            out << item.foo << ":" << item.bar;
            first = false;
        }
    };
    /// [Example Adding Custom Formatting]
    EXPECT_EQ("Items: 1:x; 2:y", LoggedText());
}

TEST_F(LoggingTest, LoggingNestedStructures) {
    struct Item {
        int foo;
        std::string bar;
    };
    std::vector<Item> list{{1, "x"}, {2, "y"}};

    /// [Example Logging Nested Structures]
    LOG_INFO() << [&list](auto& out) {
        out << "Records: [";
        for (const auto& item : list) {
            out << "{foo=" << item.foo << ", bar=" << item.bar << "}, ";
        }
        out << "]";
    };
    EXPECT_EQ("Records: [{foo=1, bar=x}, {foo=2, bar=y}, ]", LoggedText());
    ClearLog();

    LOG_INFO() << [&list](auto& out) {
        out << "Records: [";
        for (const auto& item : list) {
            out.Format("{{foo={}, bar={}}}, ", item.foo, item.bar);
        }
        out << "]";
    };
    EXPECT_EQ("Records: [{foo=1, bar=x}, {foo=2, bar=y}, ]", LoggedText());
    /// [Example Logging Nested Structures]
}

USERVER_NAMESPACE_END
