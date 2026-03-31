#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/protobuf/json/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

TEST(ExceptionsTest, CtorCorrectlyInitializesObject) {
    {
        const std::vector<std::pair<ParseErrorCode, std::string>> errors{
            {ParseErrorCode::kUnknownField, "hello world"},
            {ParseErrorCode::kUnknownEnum, ""},
            {ParseErrorCode::kMultipleOneofFields, "additional description"},
            {ParseErrorCode::kInvalidType, ""},
            {ParseErrorCode::kInvalidValue, ""},
            {static_cast<ParseErrorCode>(100), ""}
        };
        const std::string path = "field.array[0].item[1]";

        for (const auto& e : errors) {
            ParseError error(e.first, path, e.second);

            EXPECT_EQ(error.GetErrorInfo().GetCode(), e.first);
            EXPECT_EQ(error.GetErrorInfo().GetPath(), path);
            EXPECT_EQ(std::move(error).GetErrorInfo().GetPath(), path);
        }
    }

    {
        const std::vector<std::pair<PrintErrorCode, std::string>>
            errors{{PrintErrorCode::kInvalidValue, ""}, {static_cast<PrintErrorCode>(100), "hello world"}};
        const std::string path = "field.array[0].item[1].map['key']";

        for (const auto& e : errors) {
            PrintError error(e.first, path, e.second);

            EXPECT_EQ(error.GetErrorInfo().GetCode(), e.first);
            EXPECT_EQ(error.GetErrorInfo().GetPath(), path);
            EXPECT_EQ(std::move(error).GetErrorInfo().GetPath(), path);
        }
    }
}

TEST(ExceptionsTest, WhatContainsImportantDetails) {
    {
        const std::string path = "field.array[0].item[1]";
        const std::string additional_desc = "hello world";
        const ParseError error(ParseErrorCode::kInvalidType, path, additional_desc);

        EXPECT_THAT(error.what(), ::testing::HasSubstr(path));
        EXPECT_THAT(error.what(), ::testing::HasSubstr(additional_desc));
    }

    {
        const std::string path = "field.array[0].item[1].map['key']";
        const std::string additional_desc = "hello world";
        const PrintError error(PrintErrorCode::kInvalidValue, path, additional_desc);

        EXPECT_THAT(error.what(), ::testing::HasSubstr(path));
        EXPECT_THAT(error.what(), ::testing::HasSubstr(additional_desc));
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
