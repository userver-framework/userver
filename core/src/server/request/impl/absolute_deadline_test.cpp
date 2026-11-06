#include <userver/server/request/impl/absolute_deadline.hpp>

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using server::request::TaskInheritedOriginalDeadline;
using server::request::impl::ParseXRequestDeadlineString;
using server::request::impl::TryMakeDeadlinePreferringAbsoluteTimestamp;

constexpr auto kMaxOriginalDeadline = std::chrono::time_point_cast<
    TaskInheritedOriginalDeadline::duration>(std::chrono::system_clock::time_point::max());

const std::string kMaxOriginalDeadlineHeader = std::to_string(kMaxOriginalDeadline.time_since_epoch().count());

const std::string kOutOfRangeHeaders[] = {
    std::to_string(static_cast<std::uint64_t>(kMaxOriginalDeadline.time_since_epoch().count()) + 1),
    std::to_string(static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1),
    std::to_string(std::numeric_limits<std::uint64_t>::max()),
};

const std::optional<std::chrono::milliseconds> kClientTimeouts[] = {
    std::nullopt,
    std::chrono::milliseconds{5000},
};

}  // namespace

TEST(ParseXRequestDeadlineString, ParsesMicrosecondsSinceEpoch) {
    EXPECT_EQ(
        ParseXRequestDeadlineString("1700000000123456"),
        TaskInheritedOriginalDeadline{std::chrono::microseconds{1700000000123456}}
    );
    EXPECT_EQ(ParseXRequestDeadlineString(kMaxOriginalDeadlineHeader), kMaxOriginalDeadline);
}

TEST(ParseXRequestDeadlineString, RejectsInvalid) {
    EXPECT_EQ(ParseXRequestDeadlineString(""), std::nullopt);
    EXPECT_EQ(ParseXRequestDeadlineString("not-a-timestamp"), std::nullopt);
    EXPECT_EQ(ParseXRequestDeadlineString("-1"), std::nullopt);
}

TEST(ParseXRequestDeadlineString, RejectsOutOfSystemClockRange) {
    for (const auto& header : kOutOfRangeHeaders) {
        SCOPED_TRACE(header);
        EXPECT_EQ(ParseXRequestDeadlineString(header), std::nullopt);
    }
}

UTEST(TryMakeDeadlinePreferringAbsoluteTimestamp, IgnoresOutOfSystemClockRangeHeader) {
    for (const auto& header : kOutOfRangeHeaders) {
        for (const auto& client_timeout : kClientTimeouts) {
            SCOPED_TRACE(header);
            SCOPED_TRACE(client_timeout.has_value() ? "with client timeout" : "without client timeout");
            EXPECT_EQ(
                TryMakeDeadlinePreferringAbsoluteTimestamp(
                    ParseXRequestDeadlineString(header),
                    true,
                    client_timeout,
                    dynamic_config::GetDefaultSnapshot(),
                    nullptr
                ),
                std::nullopt
            );
        }
    }
}

UTEST(TryMakeDeadlinePreferringAbsoluteTimestamp, MaxSystemClockDeadlineIsNotReached) {
    const auto deadline = TryMakeDeadlinePreferringAbsoluteTimestamp(
        ParseXRequestDeadlineString(kMaxOriginalDeadlineHeader),
        true,
        std::nullopt,
        dynamic_config::GetDefaultSnapshot(),
        nullptr
    );
    ASSERT_TRUE(deadline.has_value());
    EXPECT_FALSE(deadline->IsReached());
}

USERVER_NAMESPACE_END
