#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/protobuf/json/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

TEST(ExceptionsTest, CtorCorrectlyInitializesObject) {
    {
        const std::string path = "field.array[0].item[1]";
        const std::vector<ParseErrorCode> codes{
            ParseErrorCode::kUnknownField,
            ParseErrorCode::kUnknownEnum,
            ParseErrorCode::kMultipleOneofFields,
            ParseErrorCode::kInvalidType,
            ParseErrorCode::kInvalidValue,
            static_cast<ParseErrorCode>(100)
        };

        for (const auto& code : codes) {
            ParseError error(code, path);

            EXPECT_EQ(error.GetErrorInfo().GetCode(), code);
            EXPECT_EQ(error.GetErrorInfo().GetPath(), path);
            EXPECT_EQ(std::move(error).GetErrorInfo().GetPath(), path);
        }
    }

    {
        const std::string path = "field.array[0].item[1].map['key']";
        const std::vector<PrintErrorCode> codes{PrintErrorCode::kInvalidValue, static_cast<PrintErrorCode>(100)};

        for (const auto& code : codes) {
            PrintError error(code, path);

            EXPECT_EQ(error.GetErrorInfo().GetCode(), code);
            EXPECT_EQ(error.GetErrorInfo().GetPath(), path);
            EXPECT_EQ(std::move(error).GetErrorInfo().GetPath(), path);
        }
    }
}

TEST(ExceptionsTest, WhatContainsImportantDetails) {
    {
        const std::string path = "field.array[0].item[1]";
        const ParseError error(ParseErrorCode::kInvalidType, path);

        EXPECT_THAT(error.what(), ::testing::HasSubstr(path));
    }

    {
        const std::string path = "field.array[0].item[1].map['key']";
        const PrintError error(PrintErrorCode::kInvalidValue, path);

        EXPECT_THAT(error.what(), ::testing::HasSubstr(path));
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
