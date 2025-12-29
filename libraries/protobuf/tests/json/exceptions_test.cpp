#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/protobuf/json/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace protobuf::json::tests {

TEST(ExceptionsTest, CtorCorrectlyInitializesObject) {
    {
        const std::string path = "field.array[0].item[1]";
        const std::vector<ReadErrorCode> codes{
            ReadErrorCode::kUnknownField,
            ReadErrorCode::kUnknownEnum,
            ReadErrorCode::kMultipleOneofFields,
            ReadErrorCode::kInvalidType,
            ReadErrorCode::kInvalidValue,
            static_cast<ReadErrorCode>(100)
        };

        for (const auto& code : codes) {
            ReadError error(code, path);

            EXPECT_EQ(error.GetErrorInfo().GetCode(), code);
            EXPECT_EQ(error.GetErrorInfo().GetPath(), path);
            EXPECT_EQ(std::move(error).GetErrorInfo().GetPath(), path);
        }
    }

    {
        const std::string path = "field.array[0].item[1].map['key']";
        const std::vector<WriteErrorCode> codes{WriteErrorCode::kInvalidValue, static_cast<WriteErrorCode>(100)};

        for (const auto& code : codes) {
            WriteError error(code, path);

            EXPECT_EQ(error.GetErrorInfo().GetCode(), code);
            EXPECT_EQ(error.GetErrorInfo().GetPath(), path);
            EXPECT_EQ(std::move(error).GetErrorInfo().GetPath(), path);
        }
    }
}

TEST(ExceptionsTest, WhatContainsImportantDetails) {
    {
        const std::string path = "field.array[0].item[1]";
        const ReadError error(ReadErrorCode::kInvalidType, path);

        EXPECT_THAT(error.what(), ::testing::HasSubstr(path));
    }

    {
        const std::string path = "field.array[0].item[1].map['key']";
        const WriteError error(WriteErrorCode::kInvalidValue, path);

        EXPECT_THAT(error.what(), ::testing::HasSubstr(path));
    }
}

}  // namespace protobuf::json::tests

USERVER_NAMESPACE_END
