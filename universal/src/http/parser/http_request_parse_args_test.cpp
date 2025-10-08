#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include <userver/http/parser/http_request_parse_args.hpp>
#include <utils/impl/internal_tag.hpp>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

using namespace http::parser;


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

USERVER_NAMESPACE_END
