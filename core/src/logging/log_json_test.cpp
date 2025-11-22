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

TEST_F(LoggingJsonTest, LogExtraBool) {
    // Force thread_id and task_id to appear.
    SetDefaultLoggerLevel(logging::Level::kDebug);

    LOG_CRITICAL() << "test" << logging::LogExtra{{"bool_true", true}, {"bool_false", false}};

    auto str = GetStreamString();
    auto json = formats::json::FromString(str);

    EXPECT_EQ(str.back(), '\n');

    EXPECT_EQ(json["level"].As<std::string>(), "CRITICAL");
    EXPECT_NO_THROW(json["module"].As<std::string>());
    EXPECT_NO_THROW(json["timestamp"].As<std::string>());
    EXPECT_EQ(json["task_id"].As<std::string>(), "0");
    EXPECT_EQ(json["text"].As<std::string>(), "test");
    EXPECT_NO_THROW(json["thread_id"].As<std::string>());

    EXPECT_TRUE(json["bool_true"].As<bool>());
    EXPECT_FALSE(json["bool_false"].As<bool>());
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
    logging::JsonString object_as_str{object};

    logging::LogExtra extra{
        {"object0", object},         // avoids implicit JsonString constructor and copy
        {"object1", object_as_str},  // avoids copy of object_as_str
        {"int42", 42},
    };
    extra.Extend("object2", object);  // implicit JsonString constructor with moving out value

    extra.Extend({
        {"object3", object},                       // avoids implicit JsonString constructor and copy
        {"object4", object_as_str},                // avoids copy of object_as_str
        {"object5", logging::JsonString{object}},  // is not optimized for now :(
        {"int43", 43},
    });
    extra.Extend({"object6", object});
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
    EXPECT_EQ(json["int42"].As<int>(), 42);
    EXPECT_EQ(json["int43"].As<int>(), 43);

    for (auto object : {"object0", "object1", "object2", "object3", "object4", "object5", "object6"}) {
        EXPECT_TRUE(json[object].IsObject()) << "For key '" << object << "' the value is " << ToString(json[object]);
        EXPECT_EQ(json[object]["inner"]["number"].As<int>(), 10);
    }

    EXPECT_TRUE(json["null_object"].IsNull());
}

USERVER_NAMESPACE_END
