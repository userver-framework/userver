#include <userver/utils/string_to_duration.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct data_t {
  const char* const name;
  const char* const data;
  const std::chrono::milliseconds ethalon;
};

inline std::string PrintToString(const data_t& d) { return d.name; }

using TestData = std::initializer_list<data_t>;

}  // namespace

////////////////////////////////////////////////////////////////////////////////

class StringToDuration : public ::testing::TestWithParam<data_t> {};

INSTANTIATE_TEST_SUITE_P(
    /*no prefix*/, StringToDuration,
    ::testing::ValuesIn(TestData{
        {"milliseconds", "103ms", std::chrono::milliseconds(103)},
        {"milliseconds_huge", "938493ms", std::chrono::milliseconds(938493)},
        {"milliseconds_zero", "0ms", std::chrono::milliseconds(0)},

        {"seconds", "13s", std::chrono::seconds(13)},
        {"seconds_huge", "12093843s", std::chrono::seconds(12093843)},
        {"seconds_zero", "0s", std::chrono::seconds(0)},

        {"minutes", "13m", std::chrono::minutes(13)},
        {"minutes_huge", "12093843m", std::chrono::minutes(12093843)},
        {"minutes_zero", "0m", std::chrono::minutes(0)},

        {"hours", "3h", std::chrono::hours(3)},
        {"hours_huge", "120938439h", std::chrono::hours(120938439)},
        {"hours_zero", "0h", std::chrono::hours(0)},

        {"days", "3d", std::chrono::hours(3 * 24)},
        {"days_huge", "938439d", std::chrono::hours(938439 * 24)},
        {"days_zero", "0d", std::chrono::hours(0)},
    }),
    ::testing::PrintToStringParamName());

TEST_P(StringToDuration, Basic) {
  const auto p = GetParam();
  std::chrono::milliseconds ms = utils::StringToDuration(p.data);
  EXPECT_EQ(ms, p.ethalon);
}

TEST(StringToDurationError, Throw) {
  EXPECT_ANY_THROW(utils::StringToDuration("999999999999d"));

  EXPECT_ANY_THROW(
      utils::StringToDuration("99999999999999999999999999999999999ms"));

  EXPECT_ANY_THROW(utils::StringToDuration("1z"));
  EXPECT_ANY_THROW(utils::StringToDuration("h"));
  EXPECT_ANY_THROW(utils::StringToDuration("s"));
  EXPECT_ANY_THROW(utils::StringToDuration(""));
}

USERVER_NAMESPACE_END
