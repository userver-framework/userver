#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <logging/logging_test.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingJsonTest, Smoke) {
    // Force thread_id and task_id to appear.
    SetDefaultLoggerLevel(logging::Level::kDebug);

    LOG_CRITICAL() << "foo\nbar\rbaz";

    auto str = GetStreamString();
    auto json = formats::json::FromString(str);

    EXPECT_EQ(str.back(), '\n');

    EXPECT_EQ(json["level"].As<std::string>(), "CRITICAL");
    EXPECT_NO_THROW(json["module"].As<std::string>());
    EXPECT_NO_THROW(json["timestamp"].As<std::string>());
    EXPECT_EQ(json["task_id"].As<std::string>(), "0");
    EXPECT_EQ(json["text"].As<std::string>(), "foo\nbar\rbaz");
    EXPECT_NO_THROW(json["thread_id"].As<std::string>());
}

TEST_F(LoggingJsonTest, NoThreadIdInProduction) {
    SetDefaultLoggerLevel(logging::Level::kInfo);

    LOG_CRITICAL() << "foo";

    const auto str = GetStreamString();
    const auto json = formats::json::FromString(str);

    EXPECT_FALSE(json.HasMember("task_id")) << ToString(json);
    EXPECT_FALSE(json.HasMember("thread_id")) << ToString(json);
}

TEST_F(LoggingJsonTest, LogExtraJsonString) {
    // Force thread_id and task_id to appear.
    SetDefaultLoggerLevel(logging::Level::kDebug);

    using formats::literals::operator""_json;

    auto object = R"({
        "inner": {
            "number": 10
        }
    })"_json;

    logging::LogExtra extra;
    extra.Extend("object", object);  // implicit JsonString constructor
    extra.Extend("null_object", logging::JsonString());

    LOG_CRITICAL() << extra;

    auto str = GetStreamString();
    auto json = formats::json::FromString(str);

    EXPECT_EQ(str.back(), '\n');

    EXPECT_EQ(json["level"].As<std::string>(), "CRITICAL");
    EXPECT_NO_THROW(json["module"].As<std::string>());
    EXPECT_NO_THROW(json["timestamp"].As<std::string>());
    EXPECT_EQ(json["task_id"].As<std::string>(), "0");
    EXPECT_EQ(json["text"].As<std::string>(), "");
    EXPECT_NO_THROW(json["thread_id"].As<std::string>());

    EXPECT_TRUE(json["object"].IsObject());
    EXPECT_EQ(json["object"]["inner"]["number"].As<int>(), 10);
    EXPECT_TRUE(json["null_object"].IsNull());
}

USERVER_NAMESPACE_END
