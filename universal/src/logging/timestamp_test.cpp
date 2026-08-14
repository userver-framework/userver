#include <userver/logging/timestamp.hpp>

#include <chrono>
#include <ctime>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

TEST(Timestamp, GMTimeString) {
    const auto time = std::chrono::system_clock::from_time_t(1);
    EXPECT_EQ(logging::GetCurrentGMTimeString(time).ToStringView(), "1970-01-01T00:00:01");
}

TEST(Timestamp, LocalTimeString) {
    std::tm local_time{};
    local_time.tm_sec = 56;
    local_time.tm_min = 34;
    local_time.tm_hour = 12;
    local_time.tm_mday = 2;
    local_time.tm_mon = 0;
    local_time.tm_year = 120;
    local_time.tm_isdst = -1;

    const auto time = std::chrono::system_clock::from_time_t(std::mktime(&local_time));
    EXPECT_EQ(logging::GetCurrentLocalTimeString(time).ToStringView(), "2020-01-02T12:34:56");
}

TEST(Timestamp, CacheIsUpdated) {
    const auto first = std::chrono::system_clock::from_time_t(1'700'000'000);
    const auto second = first + std::chrono::seconds{1};

    EXPECT_NE(
        logging::GetCurrentGMTimeString(first).ToStringView(),
        logging::GetCurrentGMTimeString(second).ToStringView()
    );
    EXPECT_NE(
        logging::GetCurrentLocalTimeString(first).ToStringView(),
        logging::GetCurrentLocalTimeString(second).ToStringView()
    );
}

}  // namespace

USERVER_NAMESPACE_END
