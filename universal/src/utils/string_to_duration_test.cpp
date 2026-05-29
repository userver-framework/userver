#include <userver/utils/string_to_duration.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct DataT {
    const char* const name;
    const char* const data;
    const std::chrono::milliseconds ethalon;
};

inline std::string PrintToString(const DataT& d) { return d.name; }

using TestData = std::initializer_list<DataT>;

}  // namespace

////////////////////////////////////////////////////////////////////////////////

class StringToDuration : public ::testing::TestWithParam<DataT> {};

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/,
    StringToDuration,
    ::testing::ValuesIn(TestData{
        {.name = "milliseconds", .data = "103ms", .ethalon = std::chrono::milliseconds(103)},
        {.name = "milliseconds_huge", .data = "938493ms", .ethalon = std::chrono::milliseconds(938493)},
        {.name = "milliseconds_zero", .data = "0ms", .ethalon = std::chrono::milliseconds(0)},

        {.name = "seconds", .data = "13s", .ethalon = std::chrono::seconds(13)},
        {.name = "seconds_huge", .data = "12093843s", .ethalon = std::chrono::seconds(12093843)},
        {.name = "seconds_zero", .data = "0s", .ethalon = std::chrono::seconds(0)},

        {.name = "minutes", .data = "13m", .ethalon = std::chrono::minutes(13)},
        {.name = "minutes_huge", .data = "12093843m", .ethalon = std::chrono::minutes(12093843)},
        {.name = "minutes_zero", .data = "0m", .ethalon = std::chrono::minutes(0)},

        {.name = "hours", .data = "3h", .ethalon = std::chrono::hours(3)},
        {.name = "hours_huge", .data = "120938439h", .ethalon = std::chrono::hours(120938439)},
        {.name = "hours_zero", .data = "0h", .ethalon = std::chrono::hours(0)},

        {.name = "days", .data = "3d", .ethalon = std::chrono::hours(3 * 24)},
        {.name = "days_huge", .data = "938439d", .ethalon = std::chrono::hours(938439 * 24)},
        {.name = "days_zero", .data = "0d", .ethalon = std::chrono::hours(0)},
    }),
    ::testing::PrintToStringParamName()
);

TEST_P(StringToDuration, Basic) {
    const auto p = GetParam();
    const std::chrono::milliseconds ms = utils::StringToDuration(p.data);
    EXPECT_EQ(ms, p.ethalon);
}

TEST(StringToDurationCheck, NotNullTerminated) {
    std::string_view minutes{"42ms"};
    minutes.remove_suffix(1);
    const auto ms = utils::StringToDuration(minutes);
    EXPECT_EQ(ms, std::chrono::minutes(42));
}

TEST(StringToDurationError, Throw) {
    EXPECT_ANY_THROW(utils::StringToDuration("999999999999d"));

    EXPECT_ANY_THROW(utils::StringToDuration("99999999999999999999999999999999999ms"));

    EXPECT_ANY_THROW(utils::StringToDuration("1z"));
    EXPECT_ANY_THROW(utils::StringToDuration("h"));
    EXPECT_ANY_THROW(utils::StringToDuration("s"));
    EXPECT_ANY_THROW(utils::StringToDuration(""));
    EXPECT_ANY_THROW(utils::StringToDuration("-1z"));
    EXPECT_ANY_THROW(utils::StringToDuration("-h"));
    EXPECT_ANY_THROW(utils::StringToDuration("-s"));
    EXPECT_ANY_THROW(utils::StringToDuration("-"));
}

USERVER_NAMESPACE_END
