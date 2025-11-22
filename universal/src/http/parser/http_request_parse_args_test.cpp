#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <userver/http/parser/http_request_parse_args.hpp>
#include <utils/impl/internal_tag.hpp>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

using http::parser::ParseArgs;

TEST(ParseArgs, Basic) {
    constexpr std::string_view params = "a=123&b=456&c1=someText&x";
    std::unordered_map<std::string, std::vector<std::string>, utils::StrCaseHash> result;
    ParseArgs(params, result);

    EXPECT_EQ(result.at("a").size(), 1);
    EXPECT_EQ(result.at("a")[0], "123");
    EXPECT_EQ(result.at("b")[0], "456");
    EXPECT_EQ(result.at("c1")[0], "someText");
    EXPECT_EQ(result.at("x")[0], "");
}

TEST(ParseArgs, Encoding) {
    constexpr std::string_view params = "a=%20&x";
    std::unordered_map<std::string, std::vector<std::string>, utils::StrCaseHash> result;
    ParseArgs(params, result);

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result.at("a").size(), 1);
    EXPECT_EQ(result.at("a")[0], " ");
    EXPECT_EQ(result.at("x").size(), 1);
    EXPECT_EQ(result.at("x")[0], "");
}

TEST(ParseArgs, EncodingKeys) {
    constexpr std::string_view params = "%20=a&a=%200&%20";
    std::unordered_map<std::string, std::vector<std::string>, utils::StrCaseHash> result;
    ParseArgs(params, result);

    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result.at(" ").size(), 2);
    EXPECT_EQ(result.at(" ")[0], "a");
    EXPECT_EQ(result.at(" ")[1], "");
    EXPECT_EQ(result.at("a").size(), 1);
    EXPECT_EQ(result.at("a")[0], " 0");
}

TEST(ParseArgs, EmptyKeys) {
    std::unordered_map<std::string, std::vector<std::string>, utils::StrCaseHash> result;

    ParseArgs("", result);
    EXPECT_TRUE(result.empty());

    result.clear();
    ParseArgs("a=123&", result);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result.at("a")[0], "123");

    result.clear();
    ParseArgs("a=123&b=1&b=2", result);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result.at("a").size(), 1);
    EXPECT_EQ(result.at("a")[0], "123");
    EXPECT_EQ(result.at("b").size(), 2);
    EXPECT_EQ(result.at("b")[0], "1");
    EXPECT_EQ(result.at("b")[1], "2");

    result.clear();
    ParseArgs("a&b&b", result);
    EXPECT_EQ(result.size(), 2);
    EXPECT_EQ(result.at("a").size(), 1);
    EXPECT_EQ(result.at("a")[0], "");
    EXPECT_EQ(result.at("b").size(), 2);
    EXPECT_EQ(result.at("b")[0], "");
    EXPECT_EQ(result.at("b")[1], "");

    result.clear();
    ParseArgs("a&b=&b&c", result);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result.at("a").size(), 1);
    EXPECT_EQ(result.at("a")[0], "");
    EXPECT_EQ(result.at("b").size(), 2);
    EXPECT_EQ(result.at("b")[0], "");
    EXPECT_EQ(result.at("b")[1], "");
    EXPECT_EQ(result.at("c").size(), 1);
    EXPECT_EQ(result.at("c")[0], "");
}

USERVER_NAMESPACE_END
