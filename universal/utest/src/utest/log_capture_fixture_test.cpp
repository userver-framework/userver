#include <userver/utest/log_capture_fixture.hpp>

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kLine1 = "tskv\ttext=hello\tlevel=INFO\n";
constexpr std::string_view kLine2 = "tskv\ttext=world\tlevel=DEBUG\n";

}  // namespace

TEST(ParseTskvLogRecords, Empty) { EXPECT_THAT(utest::ParseTskvLogRecords(""), testing::IsEmpty()); }

TEST(ParseTskvLogRecords, SingleRecord) {
    const auto records = utest::ParseTskvLogRecords(kLine1);
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].GetText(), "hello");
    EXPECT_EQ(records[0].GetTagOptional("level"), "INFO");
    EXPECT_EQ(records[0].GetLogRaw(), kLine1);
}

TEST(ParseTskvLogRecords, MultipleRecords) {
    const std::string content = std::string(kLine1) + std::string(kLine2);
    const auto records = utest::ParseTskvLogRecords(content);
    ASSERT_EQ(records.size(), 2);
    EXPECT_EQ(records[0].GetText(), "hello");
    EXPECT_EQ(records[1].GetText(), "world");
    EXPECT_EQ(records[0].GetLogRaw(), kLine1);
    EXPECT_EQ(records[1].GetLogRaw(), kLine2);
}

TEST(ParseTskvLogRecords, NoTrailingNewline) {
    UEXPECT_THROW_MSG(
        utest::ParseTskvLogRecords("tskv\ttext=hello"),
        std::runtime_error,
        R"(TSKV log contents must end with '\n')"
    );
}

TEST(ParseTskvLogRecords, LeftoverAfterLastNewline) {
    UEXPECT_THROW_MSG(utest::ParseTskvLogRecords("tskv\ttext=hello\nleftover"), std::runtime_error, "leftover");
}

TEST(ParseTskvLogRecords, InvalidRecord) {
    UEXPECT_THROW_MSG(utest::ParseTskvLogRecords("garbage\n"), std::runtime_error, "Invalid log record");
}

USERVER_NAMESPACE_END
