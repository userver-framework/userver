#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <logging/logging_test.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

TEST_F(LoggingJsonTest, Smoke) {
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

USERVER_NAMESPACE_END
